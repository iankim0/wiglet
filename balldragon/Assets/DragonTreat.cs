using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class DragonTreat : MonoBehaviour
{

    public OVRHand leftHand;
    public OVRHand rightHand;
    private OVRHand currHand;
    private GestureTracker leftGesture;
    private GestureTracker rightGesture;
    private GestureTracker currGesture;

    private Rigidbody t_rigidbody;

    public bool held;

    // Start is called before the first frame update
    void Start()
    {
        gameObject.SetActive(false);

        leftGesture = leftHand.GetComponent<GestureTracker>();
        rightGesture = rightHand.GetComponent<GestureTracker>();

        t_rigidbody = gameObject.GetComponent<Rigidbody>();
    }

    void OnEnable()
    {
        currGesture = leftGesture;
        currHand = leftHand;
        transform.SetParent(currHand.transform, true);
        transform.eulerAngles = new Vector3(0.0f, 0.0f, 0.0f);
        held = true;
    }


    // Update is called once per frame
    void Update()
    {
        if (gameObject.activeSelf && held)
        {
            held = currGesture.pinching;
        }

        if (held)
        {
            t_rigidbody.constraints = RigidbodyConstraints.FreezeAll;
            transform.position = currGesture.indexTip + currHand.PointerPose.forward * 0.01f;
        }
        else
        {
            transform.parent = null;
            if (t_rigidbody.constraints == RigidbodyConstraints.FreezeAll)
            {
                t_rigidbody.constraints = RigidbodyConstraints.None;
            }
        }
    }
}
