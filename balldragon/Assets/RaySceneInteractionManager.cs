using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class RaySceneInteractionManager : MonoBehaviour
{
    private OVRCameraRig cameraRig;
    private GameObject ghostSphere;
    private GameObject ghostArm;
    private LineRenderer lineRenderer;

    public OVRHand leftHand;
    public OVRHand rightHand;
    private GestureTracker leftGesture;
    private GestureTracker rightGesture;

    public Material selectionGhost;

    public GameObject dragon;
    public GameObject testBall;

    bool xDown;
    bool aDown;

    bool anchored;

    bool passthrough;

    private void setPassthrough(bool pt)
    {
        OVRPassthroughLayer passthroughLayer = cameraRig.GetComponent<OVRPassthroughLayer>();
        passthroughLayer.enabled = pt;
        Camera.main.clearFlags = pt ? CameraClearFlags.SolidColor : CameraClearFlags.Skybox;
        if (pt) cameraRig.centerEyeAnchor.GetComponent<Camera>().backgroundColor = Color.clear;
        cameraRig.GetComponent<OVRManager>().isInsightPassthroughEnabled = pt;
    }

    private void Awake()
    {
        cameraRig = FindObjectOfType<OVRCameraRig>();
        //setPassthrough(false);
        setPassthrough(true);
        OVRInput.EnableSimultaneousHandsAndControllers();

        lineRenderer = GetComponent<LineRenderer>();
        lineRenderer.alignment = LineAlignment.View;

        anchored = false;

        ghostSphere = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        ghostSphere.transform.localScale = new Vector3(0.1f, 0.1f, 0.1f);
        ghostSphere.GetComponent<Collider>().enabled = false;
        ghostSphere.GetComponent<Renderer>().material = selectionGhost;
        ghostSphere.SetActive(false);

        dragon.SetActive(false);

        leftGesture = leftHand.GetComponent<GestureTracker>();
        rightGesture = rightHand.GetComponent<GestureTracker>();
    }

    private Ray GetControllerRay()
    {
        Vector3 rayOrigin;
        Vector3 rayDirection;
        if ((OVRInput.activeControllerType & OVRInput.Controller.LTouch) == OVRInput.Controller.LTouch)
        {
            rayOrigin = cameraRig.leftControllerInHandAnchor.position;
            rayDirection = cameraRig.leftControllerInHandAnchor.forward;
        }
        else
        {
            rayOrigin = leftGesture.indexTip;
            rayDirection = leftHand.PointerPose.forward;
        }

        return new Ray(rayOrigin, rayDirection);
    }

    // Update is called once per frame
    public void Update()
    {

        // Not currently using  pinch input

        xDown = OVRInput.GetDown(OVRInput.RawButton.X) && !leftGesture.pinchDown;
        aDown = OVRInput.GetDown(OVRInput.RawButton.A) && !rightGesture.pinchDown;

        if (OVRInput.GetDown(OVRInput.Button.PrimaryThumbstick)) { // reset button
            anchored = false;
            dragon.GetComponent<dragon>().pivotPointSet = false;
            dragon.SetActive(false);
        }

        /**
        if (rightGesture.pinchDown)
        {
            testBall.transform.position = rightHand.PointerPose.position + rightHand.PointerPose.forward * 0.1f;
        }
        **/

        if (dragon.activeSelf == false)
        {
            if (anchored == false)
            {
                lineRenderer.enabled = false;
                var ray = GetControllerRay();
                ghostSphere.SetActive(true);
                ghostSphere.transform.position = ray.origin + ray.direction*0.08f;
                if (xDown)
                {
                    anchored = true;
                }
            }
            else // anchored == true
            {
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

                if (xDown)
                {
                    dragon.transform.position = ghostSphere.transform.position;
                    dragon.transform.rotation = ghostSphere.transform.rotation;
                    dragon.SetActive(true);
                    lineRenderer.enabled = false;
                    ghostSphere.SetActive(false);
                }
            }
        }
        else //dragon.activeSelf == true
        {

        }

        if (OVRInput.GetDown(OVRInput.RawButton.Y)) {
            passthrough = !passthrough;
            setPassthrough(passthrough);
        }
    }
}
