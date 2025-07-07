using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class EyeTracking : MonoBehaviour
{
    public Vector3 turnTarget;
    public Vector3 eyesTarget;
    public Transform headBone;

    public Transform leftEyeBone;
    public Transform rightEyeBone;

   

    // Start is called before the first frame update
    void Start()
    {
        eyesTarget = turnTarget;
    }

    // Update is called once per frame
    void Update()
    {
       
    }

    void LateUpdate()
    {
        HeadTrackingUpdate();
        EyeTrackingUpdate();
    }

    void HeadTrackingUpdate()
    {
        // Store the current head rotation since we will be resetting it
        Quaternion currentLocalRotation = headBone.localRotation;
        // Reset the head rotation so our world to local space transformation will use the head's zero rotation.
        // Note: Quaternion.Identity is the quaternion equivalent of "zero"
        headBone.localRotation = Quaternion.identity;

        Vector3 targetWorldLookDir = turnTarget - headBone.position;
        Vector3 targetLocalLookDir = headBone.InverseTransformDirection(targetWorldLookDir);

        // Apply angle limit
        targetLocalLookDir = Vector3.RotateTowards(
            Vector3.forward,
            targetLocalLookDir,
            2.0f * Mathf.PI, // Note we multiply by Mathf.Deg2Rad here to convert degrees to radians
            0 // We don't care about the length here, so we leave it at zero
        );

        // Get the local rotation by using LookRotation on a local directional vector
        Quaternion targetLocalRotation = Quaternion.LookRotation(targetLocalLookDir, Vector3.up);

        // Apply smoothing
        headBone.localRotation = Quaternion.Slerp(
            currentLocalRotation,
            targetLocalRotation,
            0.6f
        );
    }

    void EyeTrackingUpdate()
    {

        Vector3 targetLookDir = Vector3.RotateTowards(
            headBone.forward,
            eyesTarget - headBone.position,
            Mathf.Deg2Rad * 40, // Note we multiply by Mathf.Deg2Rad here to convert degrees to radians
            0 // We don't care about the length here, so we leave it at zero
        );

        Quaternion targetRotation = Quaternion.LookRotation(targetLookDir, Vector3.up);

        leftEyeBone.rotation = targetRotation;
        rightEyeBone.rotation = targetRotation;
       
    }
}
