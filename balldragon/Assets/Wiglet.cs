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


 public OVRHand leftHand;
 public OVRHand rightHand;
 public Material selectionGhost;
 public GameObject dragon;
 public GameObject testBall;

 private GameObject ghostSphere;
 private GameObject ghostArm;

 bool xDown;
 bool aDown;
 bool anchored;

//

 private OVRCameraRig OVR_cameraRig;
 private OVRPassthroughLayer OVR_passthroughLayer;
 private OVRManager OVR_manager;

 private GestureTracker leftGesture;
 private GestureTracker rightGesture;

 private LineRenderer lineRenderer;

 void ASSERT(bool condition) {
  if (!condition) {
    Debug.Log("ASSERT failed.");
    Debug.Log(Environment.StackTrace);
    EditorApplication.isPlaying = false;
  }
 }


 bool getPassThrough() {
  OVRPassthroughLayer OVR_passthroughLayer = OVR_cameraRig.GetComponent<OVRPassthroughLayer>();

  return(OVR_passthroughLayer.enabled);
 }
 
 void setPassthrough(bool passthrough) {
  OVR_passthroughLayer.enabled = passthrough;
  Camera.main.clearFlags = passthrough ? CameraClearFlags.SolidColor : CameraClearFlags.Skybox;
  if (passthrough) OVR_cameraRig.centerEyeAnchor.GetComponent<Camera>().backgroundColor = Color.clear;
  OVR_manager.isInsightPassthroughEnabled = passthrough;
 }





 const int PHASE_PREP = 0;
 const int PHASE_HOT  = 1;
 const int PHASE_LIVE = 2;
 int phase;

 private void Awake() {
  ASSERT(2 + 2 == 5);

  phase = PHASE_PREP;

  //

  OVR_cameraRig = FindObjectOfType<OVRCameraRig>();
  OVR_passthroughLayer = OVR_cameraRig.GetComponent<OVRPassthroughLayer>();
  OVR_manager = OVR_cameraRig.GetComponent<OVRManager>();

  leftGesture = leftHand.GetComponent<GestureTracker>();
  rightGesture = rightHand.GetComponent<GestureTracker>();

  lineRenderer = GetComponent<LineRenderer>();
  lineRenderer.alignment = LineAlignment.View;

  //

  setPassthrough(true);
  OVRInput.EnableSimultaneousHandsAndControllers();

  anchored = false; // ??

  ghostSphere = GameObject.CreatePrimitive(PrimitiveType.Sphere);
  ghostSphere.transform.localScale = new Vector3(0.1f, 0.1f, 0.1f);
  ghostSphere.GetComponent<Collider>().enabled = false;
  ghostSphere.GetComponent<Renderer>().material = selectionGhost;
  ghostSphere.SetActive(false);

  dragon.SetActive(false);
 }

 

 public void Update() {

  bool A_Pressed               = (!rightGesture.pinchDown && OVRInput.GetDown(OVRInput.RawButton.A));
  bool B_Pressed               = OVRInput.GetDown(OVRInput.RawButton.B);
  bool X_Pressed               = (!leftGesture.pinchDown && OVRInput.GetDown(OVRInput.RawButton.X));
  bool Y_Pressed               = OVRInput.GetDown(OVRInput.RawButton.Y);
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

  //

  if (Y_Pressed) setPassthrough(!getPassThrough());

  if (phase == PHASE_PREP) {

  } else if (phase == PHASE_HOT) {

  } else {
   ASSERT(phase == PHASE_LIVE);

  }


  if (LeftThumbstick_Pressed) { // reset
   anchored = false;
   dragon.GetComponent<dragon>().pivotPointSet = false;
   dragon.SetActive(false);
  }

  if (!dragon.activeSelf) {
   if (!anchored) {
    lineRenderer.enabled = false;
    ghostSphere.SetActive(true);
    ghostSphere.transform.position = LeftRay.origin + LeftRay.direction*0.08f;
    if (xDown) {
     anchored = true;
    }
   } else {
    Vector2 LThumb = OVRInput.Get(OVRInput.Axis2D.PrimaryThumbstick);
    float ytrans = Mathf.Abs(LThumb.y) > 0.6f ? LThumb.y - 0.6f : 0.0f;
    ytrans *= 0.001f;
    float rotate = Mathf.Abs(LThumb.x) > 0.4f ? LThumb.x - 0.4f : 0.0f;
    rotate *= 1.5f;

    ghostSphere.transform.Translate(new Vector3(0, ytrans, 0));
    ghostSphere.transform.Rotate(0, rotate, 0);

    lineRenderer.enabled = true;
    lineRenderer.SetPosition(0, ghostSphere.transform.position);
    lineRenderer.SetPosition(1, ghostSphere.transform.position - 0.2f*ghostSphere.transform.forward);

    if (xDown) {
     dragon.transform.position = ghostSphere.transform.position;
     dragon.transform.rotation = ghostSphere.transform.rotation;
     dragon.SetActive(true);
     lineRenderer.enabled = false;
     ghostSphere.SetActive(false);
    }
   }
  }
 }
}