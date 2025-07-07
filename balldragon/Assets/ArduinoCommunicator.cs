using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using System;
using System.IO.Ports;
using System.Threading;

public class ArduinoCommunicator : MonoBehaviour
{

    SerialPort stream;
    string port = "COM7";
    Thread thread;
    float[] dataReceived;
    float[] dataToSend;
    bool running = false;

    int numFloatsToSend = 1;
    int numFloatsToRead = 2;

    public GameObject ball1;
    public GameObject ball2;

    // Start is called before the first frame update
    void Start()
    {
        dataToSend = new float[numFloatsToSend];
        dataReceived = new float[numFloatsToRead];
        //dataToSend = [0.0f, ];
        //dataReceived = [0.0f, ];

        //ball1 = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        ball1.GetComponent<Renderer>().material.color = Color.red;
        //ball2 = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        ball2.GetComponent<Renderer>().material.color = Color.blue;

        ThreadStart ts = new ThreadStart(GetData);
        thread = new Thread(ts);
        thread.Start();
    }

    private void GetData()
    {

        stream = new SerialPort(port, 115200);
        stream.Open();
        running = true;

        string buffer;
        //stream.DiscardInBuffer();

        while (running)
        {
            //if (stream.BytesToRead >= numFloatsToRead * 4)
            //{
                buffer = stream.ReadLine();
                //Debug.Log("Received: " + buffer);
                string[] stringsReceived = buffer.Split('/');
                dataReceived = new float[stringsReceived.Length];
                for (int i = 0; i < stringsReceived.Length; i++) {
                    dataReceived[i] = float.Parse(stringsReceived[i]);
                }
            //}

            /**
            buffer = new byte[4];
            Buffer.BlockCopy(dataToSend, 0, buffer, 0, buffer.Length);
            stream.Write(buffer, 0, buffer.Length);

            Debug.Log("Sent: " + dataToSend[0]);

            Debug.Log("InBuff: " + stream.BytesToRead + "; OutBuff: " + stream.BytesToWrite);
            **/
        }
    }

    public void CloseStream(SerialPort s)
    {
        thread.Abort();
        s.Close();
        Debug.Log(s.PortName + " closed");
    }

    public void OnApplicationQuit()
    {
        CloseStream(stream);
    }

    void Update()
    {
        //ball1.transform.position = new Vector3(-1, ball1.transform.position.y, 0);
        //ball2.transform.position = new Vector3(1, ball2.transform.position.y, 0);

        if (running)
        {
            //dataToSend[0] = ball1.transform.position.y;
            ball2.transform.position = new Vector3(1, dataReceived[0], 0);
        }
    }
}
