/*
1. Disable Domain Reloading (with caution):
Enter Play Mode Settings: In Unity's editor settings, you can disable domain reloading when entering Play mode in the editor settings. This can dramatically speed up entering Play mode. 
Considerations: Disabling domain reloading means you'll need to manually reset your game state if needed, as Unity won't do it automatically. You'll also need to design your code to handle static variables and other state-related issues that arise from disabling the reload. 
*/
     
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Diagnostics;
using UnityEditor;
using System;

public class Wiglet : MonoBehaviour {
 
 
 void ASSERT(bool condition) {
  if (!condition) {
   Debug.Log("ASSERT failed.");
   Debug.Log(Environment.StackTrace);
   EditorApplication.isPlaying = false;
  }
 }



 private GameObject ghostRobot;
 private GameObject robot;


 private OVRCameraRig OVR_cameraRig;
 private OVRManager OVR_manager;
 private OVRPassthroughLayer OVR_passthroughLayer;
 public OVRHand leftHand;
 public OVRHand rightHand;
 private GestureTracker leftGesture;
 private GestureTracker rightGesture;



 void togglePassThrough() {
  OVR_passthroughLayer.enabled = !OVR_passthroughLayer.enabled;
  // OVR_manager.isInsightPassthroughEnabled = passthrough;
 } 
 
 void setPassthrough(bool passthrough) {

 }
 

 bool initialized;  
 private void Awake() {
  initialized = false;

  OVRInput.EnableSimultaneousHandsAndControllers();

  OVR_cameraRig = FindObjectOfType<OVRCameraRig>();
  OVR_manager = OVR_cameraRig.GetComponent<OVRManager>();
  OVR_passthroughLayer = OVR_cameraRig.GetComponent<OVRPassthroughLayer>();

  leftGesture = leftHand.GetComponent<GestureTracker>();
  rightGesture = rightHand.GetComponent<GestureTracker>();

  ghostRobot = GameObject.CreatePrimitive(PrimitiveType.Sphere);
  ghostRobot.transform.localScale = new Vector3(0.1f, 0.1f, 0.1f);
  ghostRobot.GetComponent<Collider>().enabled = false;
  ghostRobot.GetComponent<Renderer>().material = selectionGhost;
  ghostRobot.SetActive(false);

  robot = GameObject.CreatePrimitive(PrimitiveType.Sphere);
  robot.name = "Robot";
  robot.transform.localScale = ghostRobot.transform.localScale;
  robot.transform.position = new Vector3(0f, 0f, 0f);
  robot.SetActive(false);
 }

 [SerializeField] int phase;
 public void Update() {

  bool A_Pressed               = (Input.GetKeyDown(KeyCode.A)) || (!rightGesture.pinchDown && OVRInput.GetDown(OVRInput.RawButton.A));
  bool B_Pressed               = (Input.GetKeyDown(KeyCode.B)) || OVRInput.GetDown(OVRInput.RawButton.B);
  bool X_Pressed               = (Input.GetKeyDown(KeyCode.X)) || (!leftGesture.pinchDown && OVRInput.GetDown(OVRInput.RawButton.X));
  bool Y_Pressed               = (Input.GetKeyDown(KeyCode.Y)) || OVRInput.GetDown(OVRInput.RawButton.Y);
  bool LeftThumbstick_Pressed  = OVRInput.GetDown(OVRInput.Button.PrimaryThumbstick);
  bool RightThumbstick_Pressed = OVRInput.GetDown(OVRInput.Button.SecondaryThumbstick);
  Ray LeftRay; {
   Vector3 rayOrigin;
   Vector3 rayDirection;
   if ((OVRInput.activeControllerType & OVRInput.Controller.LTouch) == OVRInput.Controller.LTouch) {
    rayOrigin = OVR_cameraRig.leftControllerInHandAnchor.position;
    rayDirection = OVR_cameraRig.leftControllerInHandAnchor.forward;
   } else {
    rayOrigin = leftGesture.indexTip;
    rayDirection = leftHand.PointerPose.forward;
   }
   LeftRay = new Ray(rayOrigin, rayDirection);
  }
  Vector2 LeftThumb = OVRInput.Get(OVRInput.Axis2D.PrimaryThumbstick);

  if (Y_Pressed) {
   togglePassThrough();
  }

  bool reset = (!initialized || LeftThumbstick_Pressed);
  if (reset) { // reset
   initialized = true;
   phase = 0;
   ghostRobot.SetActive(true);
   robot.SetActive(false);
  }

  int _phase = 0;
  Func<bool> PHASE = () => {
   return(phase == _phase++);
  };
  Func<bool> NEXT = () => {
   bool result = X_Pressed;
   if (result) ++phase; // NOTE: phase captured by reference
   return(result);
  };

  if (false) {
  } else if (PHASE()) { // prep
   ghostRobot.transform.position = (LeftRay.origin + (0.08f * LeftRay.direction));

   if (NEXT()) {

   }
  } else if (PHASE()) { // hot

   float ytrans = Mathf.Abs(LeftThumb.y) > 0.6f ? LeftThumb.y - 0.6f : 0.0f;
   ytrans *= 0.001f;
   float rotate = Mathf.Abs(LeftThumb.x) > 0.4f ? LeftThumb.x - 0.4f : 0.0f;
   rotate *= 1.5f;
   ghostRobot.transform.Translate(new Vector3(0, ytrans, 0));
   ghostRobot.transform.Rotate(0, rotate, 0);

   if (NEXT()) {
    robot.SetActive(true);
    robot.transform.position = ghostRobot.transform.position;
    robot.transform.rotation = ghostRobot.transform.rotation;
    ghostRobot.SetActive(false);
   }

  } else if (PHASE()) { // live

   if (NEXT()) {
    initialized = false;
   }

  }

  //


 }
}