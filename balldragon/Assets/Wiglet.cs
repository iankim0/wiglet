/*
1. Disable Domain Reloading (with caution):
Enter Play Mode Settings: In Unity's editor settings, you can disable domain reloading when entering Play mode in the editor settings. This can dramatically speed up entering Play mode. 
Considerations: Disabling domain reloading means you'll need to manually reset your game state if needed, as Unity won't do it automatically. You'll also need to design your code to handle static variables and other state-related issues that arise from disabling the reload. 
*/

using UnityEngine;
using UnityEngine.Diagnostics;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.IO.Pipes;
using System.IO.Ports;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using Microsoft.Win32.SafeHandles;

public class Wiglet : MonoBehaviour {

 [SerializeField] private float robotRotation;

 public GameObject robotStick;
 
 void ASSERT(bool condition) {
  if (!condition) {
   Debug.Log("ASSERT failed.");
   Debug.Log(Environment.StackTrace);
   EditorApplication.isPlaying = false;
  }
 }

 float inverseLerp(float l, float u, float pos) {
  return ((float)pos - l) / (u - l);
 }

 float lerp(float l, float u, float t) {
  return (((u - l) * t) + l);
 }

 float MagicZeroCenteredDeadBand(float x, float deadBandRadius) {
  float Abs_x = Mathf.Abs(x);
  int Sign_x = Math.Sign(x);
  float result = 0.0f;
  if (Abs_x > deadBandRadius) {
   result = Sign_x * (Abs_x - deadBandRadius);
  }
  return(result);
 }

 float MCos01(float x) {
  return(0.5f - 0.5f * Mathf.Cos(x));
 }

 private bool DISABLE_VR = true;
 private OVRCameraRig OVR_cameraRig;

 private OVRManager OVR_manager;
 private OVRPassthroughLayer OVR_passthroughLayer;
 void OVR_Init() {
  if (!DISABLE_VR) {
   OVRInput.EnableSimultaneousHandsAndControllers();;
   OVR_cameraRig = FindObjectOfType<OVRCameraRig>();
   OVR_manager = OVR_cameraRig.GetComponent<OVRManager>();
   OVR_passthroughLayer = OVR_cameraRig.GetComponent<OVRPassthroughLayer>();
  }
 }
 void OVR_togglePassThrough() {
  if (!DISABLE_VR) {
   OVR_passthroughLayer.enabled = !OVR_passthroughLayer.enabled;
  }
 } 

 private SerialPort serialPort;
 void SerialPort_Init() {
  // TODO (Jim): scan serial ports
  serialPort = new SerialPort("COM12", 115200);
  serialPort.Open(); 
 }

 private GameObject robot;
 private GameObject stick;
 private GameObject pinball;

 void Robot_Init() {
  robot = new GameObject("Robot");
  GameObject stick = Instantiate(robotStick);
  stick.transform.SetParent(robot.transform);
  stick.name = "Stick";
  stick.transform.rotation = Quaternion.Euler(90.0f, 90.0f, 0.0f);
  stick.transform.localPosition = new Vector3(0f, 0f, (0.07f / 2 - 0.015f));
  stick.GetComponent<Renderer>().material.SetColor("_Color", new Color(1.0f, 0.5f, 0.0f));
 }

 private GameObject hand;
 void FakeHand_Init() {
  if (DISABLE_VR) {
    hand = GameObject.CreatePrimitive(PrimitiveType.Sphere); 
    hand.name = "Hand";
    hand.transform.localScale = new Vector3(0.1f, 0.1f, 0.1f);
  } else {
     hand = GameObject.Find("RightHandAnchor");
  }
 }

[SerializeField] private uint TotalBytesAvail;

NamedPipeClientStream pipe;
void Pipe_Init() {
  pipe = new NamedPipeClientStream(".", "UnityPipe", PipeDirection.InOut);
  pipe.Connect();
}
[DllImport("kernel32.dll", SetLastError = true)]
static extern bool PeekNamedPipe(SafePipeHandle handle, byte[] buffer, uint nBufferSize, ref uint bytesRead, ref uint bytesAvail, ref uint bytesLeft);
int byte_for_C = 0;



 bool initialized;
 private void Awake() {
  initialized = false;

  var allObjects = Resources.FindObjectsOfTypeAll<GameObject>();
  allObjects.FirstOrDefault(obj => obj.name == "OVRCameraRig").SetActive(!DISABLE_VR);
  allObjects.FirstOrDefault(obj => obj.name == "Camera").SetActive(DISABLE_VR);

  OVR_Init();
  Robot_Init();  
  Pipe_Init();
  FakeHand_Init();
  if (DISABLE_VR) {
    robot.transform.localPosition = new Vector3(0.0f, (0.5f * robot.transform.localScale.y), 0.0f);
  }

  pinball = GameObject.CreatePrimitive(PrimitiveType.Sphere);
  pinball.name = "Pinball";
  pinball.transform.localScale = new Vector3(0.1f, 0.1f, 0.1f);
  pinball.transform.localPosition = robot.transform.position + new Vector3(0.0f, 0.0f, 0.0f);
  pinball.GetComponent<Renderer>().material.SetColor("_Color", new Color(1.0f, 0.5f, 0.0f));
 }  

 [SerializeField] int phase;
 public void Update() {
  bool A_Pressed;
  bool B_Pressed;
  bool X_Pressed;
  bool Y_Pressed;
  bool LeftThumbstick_Pressed;
  bool RightThumbstick_Pressed;
  Ray LeftRay;
  Vector2 LeftThumb;
  if (DISABLE_VR) {
   A_Pressed = Input.GetKeyDown(KeyCode.A);
   B_Pressed = Input.GetKeyDown(KeyCode.B);
   X_Pressed = Input.GetKeyDown(KeyCode.X);
   Y_Pressed = Input.GetKeyDown(KeyCode.Y);
   LeftThumbstick_Pressed  = false;
   RightThumbstick_Pressed = false;
   LeftRay = new Ray(new Vector3(), new Vector3());
   LeftThumb = new Vector3();
  } else {
   A_Pressed = (Input.GetKeyDown(KeyCode.A)) || OVRInput.GetDown(OVRInput.RawButton.A);
   B_Pressed = (Input.GetKeyDown(KeyCode.B)) || OVRInput.GetDown(OVRInput.RawButton.B);
   X_Pressed = (Input.GetKeyDown(KeyCode.X)) || OVRInput.GetDown(OVRInput.RawButton.X);
   Y_Pressed = (Input.GetKeyDown(KeyCode.Y)) || OVRInput.GetDown(OVRInput.RawButton.Y);
   LeftThumbstick_Pressed  = OVRInput.GetDown(OVRInput.Button.PrimaryThumbstick);
   RightThumbstick_Pressed = OVRInput.GetDown(OVRInput.Button.SecondaryThumbstick);
   { // LeftRay
    Vector3 rayOrigin;
    Vector3 rayDirection;
    // if ((OVRInput.activeControllerType & OVRInput.Controller.LTouch) == OVRInput.Controller.LTouch) {
     rayOrigin = OVR_cameraRig.leftControllerInHandAnchor.position;
     rayDirection = OVR_cameraRig.leftControllerInHandAnchor.forward;
    // } else {
    // rayOrigin = leftGesture.indexTip;
    // rayDirection = leftHand.PointerPose.forward;
    // }
    LeftRay = new Ray(rayOrigin, rayDirection);
   }
   LeftThumb = OVRInput.Get(OVRInput.Axis2D.PrimaryThumbstick);
  }

  if (Y_Pressed) {
   OVR_togglePassThrough();
  }

  {
    Vector3 dir = Vector3.zero;
    if (Input.GetKey(KeyCode.RightArrow)) {
      dir += new Vector3(1, 0f, 0f);
    }
    if (Input.GetKey(KeyCode.LeftArrow)) {
      dir -= new Vector3(1, 0f, 0f);
    } 
    if (Input.GetKey(KeyCode.UpArrow)) {
      dir += new Vector3(0f, 0f, 1);
    }
    if (Input.GetKey(KeyCode.DownArrow)) {
      dir -= new Vector3(0f, 0f, 1);
    }
    hand.transform.position += (0.001f * dir);
  }

  bool reset = (!initialized || LeftThumbstick_Pressed);
  if (reset) { // reset
   initialized = true;
   phase = 0;
  }

  int _phase = 0;
  Func<bool> PHASE = () => { return(phase == _phase++); };
  Func<bool> NEXT = () => {
   bool result = X_Pressed;
   if (result) {
    ++phase;
   }
   return(result);
  }; // NOTE: phase captured by reference
  if (false) {
  } else if (PHASE()) { // prep
   if (!DISABLE_VR) {
      robot.transform.localPosition = (LeftRay.origin + (0.08f * LeftRay.direction));
   }
   if (NEXT()) {
   }
  } else if (PHASE()) { // hot
   // FORNOW
   robot.transform.Translate(new Vector3(0, (0.001f * MagicZeroCenteredDeadBand(LeftThumb.y, 0.6f)), 0));
   //robot.transform.Rotate(0, (1.5f * MagicZeroCenteredDeadBand(LeftThumb.x, 0.4f)), 0);
    if (NEXT()) {

    }
  } else if (PHASE()) { // live
    robot.transform.rotation = Quaternion.Euler(0.0f, robotRotation, 0.0f);
    {
      //pipe
      uint BytesRead = 0;
      uint BytesLeftThisMessage = 0;
      while (PeekNamedPipe(pipe.SafePipeHandle, null, 0, ref BytesRead, ref TotalBytesAvail, ref BytesLeftThisMessage) && TotalBytesAvail >= 4) {  
        byte[] buffer = new byte[4]; // float is 4 bytes
        int bytesRead = pipe.Read(buffer, 0, 4);
        ASSERT(bytesRead == 4);
        float value = BitConverter.ToSingle(buffer, 0);
        robotRotation = value * 360.0f;
      }
    }

    byte[] bytesToWrite = new byte[12];
    System.Buffer.BlockCopy(System.BitConverter.GetBytes(hand.transform.position.x - robot.transform.position.x), 0, bytesToWrite, 8, 4);
    System.Buffer.BlockCopy(System.BitConverter.GetBytes(hand.transform.position.y - robot.transform.position.y), 0, bytesToWrite, 4, 4);
    System.Buffer.BlockCopy(System.BitConverter.GetBytes(hand.transform.position.z - robot.transform.position.z), 0, bytesToWrite, 0, 4);
    pipe.Write(bytesToWrite, 0, 12);
    pipe.Flush();
   if (NEXT()) {

    initialized = false;
   }
  }
 }
}
