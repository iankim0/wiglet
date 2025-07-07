using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;

public class dragon : MonoBehaviour
{

    const float BALL_RADIUS_M = 0.05f;
    const float PIVOT_HEIGHT_M = 0.0943f;
    const float ARM_LENGTH_M = 0.2f;
    Vector3 pivotPoint; // the point around which the ball is rotating
    Vector3 zeroPoint; // the point where the ball rests on the table when theta = 0
    Vector3 rotationPlaneNormal; //normal vector to the plane that the ball is in at all points of its rotation
    float armRestAngleRadians;
    float armRestAngleTurns;

    float TOP_THETA; //angle of the top moteus controller
    float BTM_THETA; //angle of the bottom moteus controller

    // The amount of feedforward torque needed to compensate for gravity when the arm is parallel to the ground
    const float TORQUE_CONSTANT = 0.25f;

    const int STATE = 0;
    const int POSITION = 1;
    const int VELOCITY = 2;
    const float TARGETED_FLOATY = 1.0f;
    const float TARGETED_WEIGHTY = 2.0f;
    const float IDLE_FLYING = 3.0f;
    const float MOTOR_OFF = 0.0f;
    const float QUIT_MOTEUS_SCRIPT = -1.0f;

    public OVRHand leftHand;
    public OVRHand rightHand;
    private OVRHand[] hands = new OVRHand[2];
    private LineRenderer lineRenderer;
    public GameObject floorManager;
    public GameObject treat;

    Thread thread;
    public String host = "127.0.0.1";
    public int connectionPort = 25001;
    float[] dataToSend = {-2.0f, 0.2f, 0.2f};
    bool running;
    //bool isApplicationQuitting = false;

    Transform centerCamera;

    bool weighty = false;
    bool liftoff = false;

    [HideInInspector]
    public bool pivotPointSet = false;

    AnimationControl dragonAnimation;

    private void Awake()
    {
        // Receive on a separate thread so Unity doesn't freeze waiting for data
        ThreadStart ts = new ThreadStart(GetData);
        thread = new Thread(ts);
        thread.Start();
        lineRenderer = GetComponent<LineRenderer>();
        lineRenderer.alignment = LineAlignment.View;

        hands[0] = leftHand;
        hands[1] = rightHand;

        dragonAnimation = gameObject.GetComponent<AnimationControl>();

        centerCamera =  FindObjectOfType<OVRCameraRig>().GetComponent<OVRCameraRig>().centerEyeAnchor;

        transform.localScale = new Vector3(BALL_RADIUS_M, BALL_RADIUS_M, BALL_RADIUS_M);
    }

    private void OnEnable()
    {
        running = true;
        TOP_THETA = -1.0f;
        BTM_THETA = -1.0f;
        lineRenderer.enabled = false;
        if (!pivotPointSet) {
            zeroPoint = transform.position;
            armRestAngleRadians = Mathf.Asin((BALL_RADIUS_M - PIVOT_HEIGHT_M) / ARM_LENGTH_M);
            armRestAngleTurns = armRestAngleRadians * 0.159f;
            pivotPoint = transform.position - transform.forward * ARM_LENGTH_M * Mathf.Cos(armRestAngleRadians)
                                            + Vector3.down * (BALL_RADIUS_M - PIVOT_HEIGHT_M);
            Vector3 u = new Vector3(zeroPoint.x-pivotPoint.x, 0, zeroPoint.z-pivotPoint.z);
            u = u.normalized;
            rotationPlaneNormal = Vector3.Normalize(Vector3.Cross(u, Vector3.up));

            floorManager.SetActive(true);
            FloorSpawner floorSpawner = floorManager.GetComponent<FloorSpawner>();
            floorSpawner.groundLevel = pivotPoint.y - PIVOT_HEIGHT_M;
            floorSpawner.objects.Add(leftHand.transform.GetChild(0));
            floorSpawner.objects.Add(rightHand.transform.GetChild(0));
            floorSpawner.objects.Add(transform);

            pivotPointSet = true;
        }
    }

    private void OnDisable () {
        dataToSend[STATE] = QUIT_MOTEUS_SCRIPT;
    }

    private void GetData()
    {

        using TcpClient client = new TcpClient(host, connectionPort);
        NetworkStream nwStream = client.GetStream();
        byte[] buffer;

        // Start listening
        while (running)
        {
            // Read data from the network stream
            buffer = new byte[client.ReceiveBufferSize];
            int bytesRead = nwStream.Read(buffer, 0, client.ReceiveBufferSize);

            // Decode the bytes into an array of floats
            float[] dataReceived = new float[bytesRead / 4];
            Buffer.BlockCopy(buffer, 0, dataReceived, 0, bytesRead);

            if (dataReceived != null && dataReceived.Length > 0)
            {
                
                TOP_THETA = dataReceived[0];
                BTM_THETA = dataReceived[1];

                buffer = new byte[dataToSend.Length * 4];
                Buffer.BlockCopy(dataToSend, 0, buffer, 0, buffer.Length);
                nwStream.Write(buffer, 0, buffer.Length);

                if(dataToSend[STATE] == QUIT_MOTEUS_SCRIPT) {
                    client.Close();
                    running = false;
                    Debug.Log("SERVER QUIT");
                }

                //Debug.Log("Sent: [" + dataToSend[STATE] + ", " + dataToSend[POSITION] + ", " + dataToSend[VELOCITY] + "]; Received: " + TOP_THETA + ", " + BTM_THETA);
            }
        }
        thread.Abort();
    }

    // Update is called once per frame
    void Update()
    {   
        Vector3 cameraPos = centerCamera.position;
        if (!weighty) dragonAnimation.turnTarget = cameraPos;
        dragonAnimation.eyesTarget = cameraPos;
        dragonAnimation.wing_speed = 0.2f;
        dragonAnimation.turnSpeed = 0.15f;

        if (TOP_THETA > -1.0f)
        {
            Vector3 u = new Vector3(zeroPoint.x-pivotPoint.x, 0, zeroPoint.z-pivotPoint.z);
            u = u.normalized;
            Vector3 v = Vector3.up;

            float thetaBiasRadians = (TOP_THETA*6.2832f)+armRestAngleRadians;
            transform.position = (pivotPoint + (ARM_LENGTH_M*Mathf.Cos(thetaBiasRadians))*u + (ARM_LENGTH_M*Mathf.Sin(thetaBiasRadians))*v);
            Quaternion tmp = transform.rotation;
            transform.RotateAround(pivotPoint, v, (BTM_THETA-0.25f)*360f);
            transform.rotation = tmp;

            lineRenderer.enabled = false;
            List<Vector3> lineVerts = new List<Vector3>();

            lineVerts.Add(transform.position);
            lineVerts.Add(pivotPoint);

            Vector3 finalTarget = transform.position;
            Vector3 finalGesturePoint = finalTarget;
            float finalTargetRanking = 9999f;

            if(treat.activeSelf)
            {
                finalTargetRanking = 0f;
                finalTarget = treat.transform.position;
                finalGesturePoint = treat.transform.position;
                dragonAnimation.turnTarget = treat.transform.position;
                dragonAnimation.eyesTarget = treat.transform.position;
            }
            else
            {
                foreach (OVRHand hand in hands)
                {
                    GestureTracker gesture = hand.GetComponent<GestureTracker>();
                    if (gesture.pinching || gesture.palmUp || (weighty && !gesture.palmDown))
                    {
                        Vector3 target = gesture.pinching ? gesture.indexTip : gesture.palmCenterPoint;
                        Vector3 targetProjected = target - Vector3.Dot(target - pivotPoint, rotationPlaneNormal) * rotationPlaneNormal;
                        float d = (targetProjected-pivotPoint).magnitude;
                        Vector3 pathPoint = pivotPoint + (targetProjected-pivotPoint).normalized * ARM_LENGTH_M;

                        if (gesture.pinching
                                && d < finalTargetRanking
                                && d > 0.125
                                && d < 0.4
                                && target.y > pivotPoint.y-PIVOT_HEIGHT_M)
                        {
                            finalTarget = targetProjected;
                            finalGesturePoint = target;
                            finalTargetRanking = d;
                            dragonAnimation.turnTarget = gesture.indexTip;
                            dragonAnimation.eyesTarget = gesture.indexTip;
                            weighty = false;
                        }
                        else if (!gesture.pinching
                                    && d+100 < finalTargetRanking
                                    && (pathPoint-target).magnitude < 0.09
                                    && targetProjected.y < pivotPoint.y+0.1
                                    && targetProjected.y > pivotPoint.y-PIVOT_HEIGHT_M)
                        {
                            finalTarget = targetProjected + 0.05f * Vector3.up;
                            finalGesturePoint = target;
                            finalTargetRanking = d + 100;
                            weighty = true;
                        }
                    }
                }
            }

            if (finalTargetRanking < 9000) // user-specified target
            {
                liftoff = false;
                Vector3 a = finalTarget - pivotPoint;
                float unclampedAngleTurns = (Vector3.Angle(u, a)/360f - armRestAngleTurns);

                if (weighty) {

                    dataToSend[POSITION] = unclampedAngleTurns;

                    if (Vector3.Distance(finalTarget, transform.position) < 0.1f) // in hand or almost in hand
                    {
                        dataToSend[STATE] = TARGETED_WEIGHTY;
                        dragonAnimation.wing_speed = 0.1f;
                    }
                    else //going to hand
                    {
                        dataToSend[STATE] = TARGETED_FLOATY;
                        dragonAnimation.wing_speed = 0.5f;
                        dragonAnimation.turnTarget = Vector3.Lerp(finalGesturePoint, cameraPos, 0.3f);
                        dragonAnimation.eyesTarget = finalGesturePoint;
                    }
                    dataToSend[VELOCITY] = 0.25f;
                }
                else // pinch or treat
                {
                    dataToSend[STATE] = TARGETED_FLOATY;
                    dataToSend[POSITION] = Mathf.Clamp(unclampedAngleTurns, 0.05f, 0.5f - 0.05f - armRestAngleTurns);
                    dataToSend[VELOCITY] = 0.75f;
                    dragonAnimation.wing_speed = Mathf.InverseLerp(0.0f, 0.1f, Mathf.Abs(TOP_THETA-dataToSend[POSITION]))*0.5f + 0.5f;
                }

                lineVerts.Add(finalTarget);
                if (finalGesturePoint != finalTarget) lineVerts.Add(finalGesturePoint);
            }
            else // no user-specified target
            {
                dataToSend[VELOCITY] = 0.25f;
                float trueMidTheta = 0.25f-armRestAngleTurns;
                if (Mathf.Abs(TOP_THETA - trueMidTheta) < 0.1) liftoff = false;
                if (weighty || liftoff) //flying out of hand
                {
                    dataToSend[STATE] = TARGETED_FLOATY;
                    dataToSend[POSITION] =  trueMidTheta;
                    liftoff = true;
                    dragonAnimation.wing_speed = Mathf.InverseLerp(0.0f, 0.25f, Mathf.Abs(TOP_THETA-dataToSend[POSITION]));
                    dragonAnimation.turnTarget = Vector3.Lerp(transform.position + Vector3.up, cameraPos, 0.6f);
                    dragonAnimation.eyesTarget = Vector3.Lerp(transform.position + Vector3.up, cameraPos, 0.3f);
                }
                else // idle flying
                {
                    dragonAnimation.wing_speed = 0.25f;
                    dataToSend[STATE] = IDLE_FLYING;
                    dataToSend[POSITION] = TOP_THETA;
                }
                weighty = false;
            }

            lineRenderer.positionCount = lineVerts.Count;
            for (int i = 0; i < lineVerts.Count; i++)
            {
                lineRenderer.SetPosition(i, lineVerts[i]);
            }
        }
    }
}
