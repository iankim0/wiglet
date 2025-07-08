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

 float MagicZeroCenteredDeadBand(float x, float deadBandRadius) {
  float Abs_x = Mathf.Abs(x);
  int Sign_x = Math.Sign(x);
  float result = 0.0f;
  if (Abs_x > deadBandRadius) {
   result = Sign_x * (Abs_x - deadBandRadius);
  }
  return(result);
 }



 private OVRCameraRig OVR_cameraRig;
 private OVRManager OVR_manager;
 private OVRPassthroughLayer OVR_passthroughLayer;

 void OVR_Init() {
  OVRInput.EnableSimultaneousHandsAndControllers();

  OVR_cameraRig = FindObjectOfType<OVRCameraRig>();
  OVR_manager = OVR_cameraRig.GetComponent<OVRManager>();
  OVR_passthroughLayer = OVR_cameraRig.GetComponent<OVRPassthroughLayer>();
 }

 void togglePassThrough() {
  OVR_passthroughLayer.enabled = !OVR_passthroughLayer.enabled;
 } 

 private GameObject robot;

 bool initialized;  
 private void Awake() {
  initialized = false;

  OVR_Init();

  robot = GameObject.CreatePrimitive(PrimitiveType.Cube);
  robot.name = "Robot";
  robot.transform.localScale = new Vector3(0.1f, 0.1f, 0.1f);
 }

 [SerializeField] int phase;
 public void Update() {

  bool A_Pressed               = (Input.GetKeyDown(KeyCode.A)) || OVRInput.GetDown(OVRInput.RawButton.A);
  bool B_Pressed               = (Input.GetKeyDown(KeyCode.B)) || OVRInput.GetDown(OVRInput.RawButton.B);
  bool X_Pressed               = (Input.GetKeyDown(KeyCode.X)) || OVRInput.GetDown(OVRInput.RawButton.X);
  bool Y_Pressed               = (Input.GetKeyDown(KeyCode.Y)) || OVRInput.GetDown(OVRInput.RawButton.Y);
  bool LeftThumbstick_Pressed  = OVRInput.GetDown(OVRInput.Button.PrimaryThumbstick);
  bool RightThumbstick_Pressed = OVRInput.GetDown(OVRInput.Button.SecondaryThumbstick);
  Ray LeftRay; {
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
  Vector2 LeftThumb = OVRInput.Get(OVRInput.Axis2D.PrimaryThumbstick);

  if (Y_Pressed) {
   togglePassThrough();
  }

  bool reset = (!initialized || LeftThumbstick_Pressed);
  if (reset) { // reset
   initialized = true;
   phase = 0;
  }

  int _phase = 0;
  Func<bool> PHASE = () => { return(phase == _phase++); };
  Func<bool> NEXT = () => { bool result = X_Pressed; if (result) ++phase; return(result); };// NOTE: phase captured by reference
  if (false) {
  } else if (PHASE()) { // prep
   robot.transform.position = (LeftRay.origin + (0.08f * LeftRay.direction));
   if (NEXT()) {
    ;
   }
  } else if (PHASE()) { // hot
   // FORNOW
   robot.transform.Translate(new Vector3(0, (0.001f * MagicZeroCenteredDeadBand(LeftThumb.y, 0.6f)), 0));
   robot.transform.Rotate(0, (1.5f * MagicZeroCenteredDeadBand(LeftThumb.x, 0.4f)), 0);
  
  if (NEXT()) {
    ;
   }
  } else if (PHASE()) { // live
   robot.transform.Rotate(2.0f, 1.0f, 0);
   if (NEXT()) {
    initialized = false;
   }
  }
 }
}
