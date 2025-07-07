using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class WristCollider : MonoBehaviour
{

    public GameObject hand;
    public bool isLeftHand;
    float c1;
    float c2;
    [HideInInspector]
    public Vector3 newHandPosition;

    // Start is called before the first frame update
    void Start()
    {
        c1 = 0.07f * (isLeftHand ? 1 : -1);
        c2 = 0.02f * (isLeftHand ? 1 : -1);
    }

    // Update is called once per frame
    void FixedUpdate()
    {
        transform.rotation = hand.transform.rotation;
        Vector3 wristToPalm = c1 * hand.transform.localScale.x * transform.right + c2 * hand.transform.localScale.y * transform.up;
        Vector3 target = hand.transform.parent.position + wristToPalm;
        gameObject.GetComponent<Rigidbody>().MovePosition(target);
        newHandPosition = transform.position - wristToPalm;
    }
}
