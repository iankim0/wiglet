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
using System.IO.Ports;
using System.Linq;

public class Wiglet : MonoBehaviour {
 
 void ASSERT(bool condition) {
  if (!condition) {
   Debug.Log("ASSERT failed.");
   Debug.Log(Environment.StackTrace);
   EditorApplication.isPlaying = false;
  }
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
  serialPort = new SerialPort("COM11", 115200);
  serialPort.Open(); 
 }

 public GameObject floorManager;
 void FloorSpawner_Init() {
   FloorSpawner floorSpawner = floorManager.GetComponent<FloorSpawner>();
   floorSpawner.groundLevel = 0f;
   floorSpawner.objects.Add(robot.transform);
 }

 private GameObject robot;
 void Robot_Init() {
  robot = GameObject.CreatePrimitive(PrimitiveType.Cube);
  robot.name = "Wobot";
  robot.transform.localScale = new Vector3(0.1f, 0.1f, 0.1f);
 }

 bool initialized;  
 private void Awake() {
  initialized = false;

  var allObjects = Resources.FindObjectsOfTypeAll<GameObject>();
  allObjects.FirstOrDefault(obj => obj.name == "OVRCameraRig").SetActive(!DISABLE_VR);
  allObjects.FirstOrDefault(obj => obj.name == "Camera").SetActive(DISABLE_VR);

  OVR_Init();
  SerialPort_Init();
  Robot_Init();
  FloorSpawner_Init();
  
 }

 [SerializeField] int phase;
 public void Update() {

  //floorManager.SetActive(true);

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


  if (serialPort.BytesToRead != 0) {
   int int_from_arduino = int.Parse(serialPort.ReadLine());
   float a = 0.1f + MCos01(int_from_arduino / 60.0f) * 0.1f;
   robot.transform.localScale = new Vector3(a, a, a);
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
   Byte[] tmp = { 0 };
   serialPort.Write(tmp, 0, 1);
   }
   return(result);
  };// NOTE: phase captured by reference
  if (false) {
  } else if (PHASE()) { // prep
   if (DISABLE_VR) {
    robot.transform.localPosition = new Vector3(0.0f, (0.5f * robot.transform.localScale.y), 0.0f);
   } else {
    robot.transform.localPosition = (LeftRay.origin + (0.08f * LeftRay.direction));
   }
   if (NEXT()) {
    
   }
  } else if (PHASE()) { // hot
   // FORNOW
   robot.transform.Translate(new Vector3(0, (0.001f * MagicZeroCenteredDeadBand(LeftThumb.y, 0.6f)), 0));
   robot.transform.Rotate(0, (1.5f * MagicZeroCenteredDeadBand(LeftThumb.x, 0.4f)), 0);
  
  if (NEXT()) {

   }
  } else if (PHASE()) { // live
   robot.transform.Rotate(2.0f, 1.0f, 0);
   if (NEXT()) {

    initialized = false;
   }
  }
 }
}
