using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class GestureTracker : MonoBehaviour
{

    public GameObject collider;

    public bool isLeftHand;
    public bool pinching;
    public bool pinchDown;
    public bool palmUp;
    public bool palmDown;
    public Vector3 palmCenterPoint;
    public bool skeletonActive;
    public Vector3 indexTip;

    private Transform palmCenterTransform;

    OVRSkeleton skeleton;

    //private GameObject ghostSphere;

    void Awake()
    {
        pinchDown = false;
        pinching = false;

        skeleton = gameObject.GetComponent<OVRSkeleton>();

        palmCenterTransform = new GameObject("PalmCenterTransform").transform;
        palmCenterTransform.parent = transform;
        palmCenterTransform.SetSiblingIndex(0);

        /***
        ghostSphere = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        ghostSphere.transform.localScale = new Vector3(0.05f, 0.05f, 0.05f);
        ghostSphere.GetComponent<Collider>().enabled = false;
        ghostSphere.SetActive(true);
        ***/
    }

    // Update is called once per frame
    void Update()
    {

        pinchDown = gameObject.GetComponent<OVRHand>().GetFingerIsPinching(OVRHand.HandFinger.Index) && !pinching;
        pinching = gameObject.GetComponent<OVRHand>().GetFingerIsPinching(OVRHand.HandFinger.Index);
        palmUp = isLeftHand ? gameObject.transform.up.y > 0.6 : gameObject.transform.up.y < -0.6;
        palmDown = isLeftHand ? gameObject.transform.up.y < -0.6 : gameObject.transform.up.y > 0.6;
        palmCenterPoint = gameObject.transform.position + (isLeftHand ? 1 : -1)*gameObject.transform.right * 0.07f * gameObject.transform.localScale.x;
        palmCenterTransform.position = palmCenterPoint;

        if (skeleton.Bones.Count > 0)
        {
            indexTip = skeleton.Bones[20].Transform.position;
            skeletonActive = true;
        }
        else skeletonActive = false;

        //if (collider.activeSelf) transform.position = collider.GetComponent<WristCollider>().newHandPosition;

        //ghostSphere.transform.position=palmCenterPoint;

    }
}
