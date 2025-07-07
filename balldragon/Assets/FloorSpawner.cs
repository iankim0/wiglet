using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class FloorSpawner : MonoBehaviour
{
    public List<Transform> objects;
    public GameObject tilePrefab;

    public Vector2Int gridSize;
    public float renderDistance;
    public float groundLevel;
    public float heightToDrawBelow;
    public float gap;

    Vector3 tilePosition;

    int xRadius;
    int zRadius;

    bool drawTiles;
    
    // Start is called before the first frame update
    void Start()
    {
        tilePosition = new Vector3();
        gameObject.SetActive(false);
    }

    // Update is called once per frame
    void Update()
    {
        xRadius = gridSize.x / 2;
        zRadius = gridSize.y / 2;

        drawTiles = false;

        foreach(Transform point in objects)
        {
            if(point.position.y < (groundLevel + heightToDrawBelow))
            {
                drawTiles = true;
            }
        }

        if (drawTiles)
        {
            DestroyChildren();
            DrawTiles(tilePrefab);
        } else
        {
            DestroyChildren();
        }
        
    }

    void DrawTiles(GameObject tilePrefab) 
    {
        foreach (Transform point in objects) 
        {
            if (point.gameObject.activeSelf)
            {
                tilePosition.x = Mathf.Floor(point.transform.position.x / gap) * gap + (xRadius * gap);
                tilePosition.y = groundLevel;
                tilePosition.z = Mathf.Floor(point.transform.position.z / gap) * gap + (zRadius * gap);

                for (int x = 0; x < gridSize.x; x++)
                {
                    tilePosition.z = Mathf.Floor(point.transform.position.z / gap) * gap + (zRadius * gap);
                    for (int z = 0; z < gridSize.y; z++)
                    {
                        float distance = Mathf.Pow(Vector3.Distance(tilePosition, point.transform.position), 2f);
                        if (distance < renderDistance)
                        {
                            GameObject tile = Instantiate(tilePrefab, tilePosition, Quaternion.identity) as GameObject;
                            tile.transform.parent = this.transform;
                            tile.transform.localScale = new Vector3(0.09f*gap, 1, 0.09f*gap);
                        }
                        tilePosition.z -= gap;
                    }
                    tilePosition.x -= gap;
                } 
            }
        }
    }

    void DestroyChildren() 
    {
        for(int i = 0; i < this.transform.childCount; i++)
        {
           GameObject child = this.transform.GetChild(i).gameObject;

           Destroy(child);
        }
    }
}
