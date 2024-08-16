#include <iostream>
#include <vector>
#include <GL/glut.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <sstream>

float rotationX = 0.0f;
float rotationY = 0.0f;
float objectTranslationX = 0.0f;
float objectTranslationY = 0.0f;
float objectTranslationZ = 0.0f;
int lastMouseX, lastMouseY;
bool isDragging = false;
bool isObjectDragging = false;

struct Triangle {
    glm::vec3 normal;
    glm::vec3 vertices[3];
};

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

glm::vec3 cameraPos(1.0f, 0.0f, 0.0f);  // 기본 카메라 위치

// Octree Node 구조체
struct OctreeNode {
    AABB box;
    std::vector<Triangle> triangles;
    OctreeNode* children[8] = { nullptr };

    ~OctreeNode() {
        for (int i = 0; i < 8; ++i) {
            delete children[i];
        }
    }
};
void renderAABB(const AABB& box);  // renderAABB 함수 프로토타입 추가
// 최대 깊이 변수
int maxDepth = 3;

// AABB의 중심점 계산
glm::vec3 calculateCenter(const AABB& box) {
    return (box.min + box.max) * 0.5f;
}

// AABB를 8개로 분할
std::vector<AABB> splitAABB(const AABB& box) {
    std::vector<AABB> children(8);
    glm::vec3 center = calculateCenter(box);

    for (int i = 0; i < 8; ++i) {
        children[i].min = glm::vec3(
            (i & 1) ? center.x : box.min.x,
            (i & 2) ? center.y : box.min.y,
            (i & 4) ? center.z : box.min.z
        );

        children[i].max = glm::vec3(
            (i & 1) ? box.max.x : center.x,
            (i & 2) ? box.max.y : center.y,
            (i & 4) ? box.max.z : center.z
        );
    }
    return children;
}

// Octree를 생성하는 재귀 함수
OctreeNode* buildOctree(const AABB& box, const std::vector<Triangle>& triangles, int depth) {
    if (depth > maxDepth || triangles.empty()) {
        return nullptr;
    }

    OctreeNode* node = new OctreeNode();
    node->box = box;
    node->triangles = triangles;

    if (depth == maxDepth) {
        return node;
    }

    std::vector<AABB> childrenAABBs = splitAABB(box);
    for (int i = 0; i < 8; ++i) {
        std::vector<Triangle> childTriangles;

        for (const auto& tri : triangles) {
            bool overlaps = false;
            for (int j = 0; j < 3; ++j) {
                if (tri.vertices[j].x >= childrenAABBs[i].min.x && tri.vertices[j].x <= childrenAABBs[i].max.x &&
                    tri.vertices[j].y >= childrenAABBs[i].min.y && tri.vertices[j].y <= childrenAABBs[i].max.y &&
                    tri.vertices[j].z >= childrenAABBs[i].min.z && tri.vertices[j].z <= childrenAABBs[i].max.z) {
                    overlaps = true;
                    break;
                }
            }
            if (overlaps) {
                childTriangles.push_back(tri);
            }
        }

        node->children[i] = buildOctree(childrenAABBs[i], childTriangles, depth + 1);
    }

    return node;
}

// Octree 렌더링 함수
void renderOctree(const OctreeNode* node) {
    if (!node) return;

    renderAABB(node->box);  // renderAABB 호출

    for (int i = 0; i < 8; ++i) {
        renderOctree(node->children[i]);
    }
}

// Octree 삭제 함수
void deleteOctree(OctreeNode* node) {
    if (!node) return;

    for (int i = 0; i < 8; ++i) {
        deleteOctree(node->children[i]);
    }

    delete node;
}

void mouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            isObjectDragging = true;
            lastMouseX = x;
            lastMouseY = y;
        }
        else if (state == GLUT_UP) {
            isObjectDragging = false;
        }
    }
    else if (button == GLUT_RIGHT_BUTTON) {
        if (state == GLUT_DOWN) {
            isDragging = true;
            lastMouseX = x;
            lastMouseY = y;
        }
        else if (state == GLUT_UP) {
            isDragging = false;
        }
    }
}

void mouseMotion(int x, int y) {
    if (isDragging) {
        rotationX += (y - lastMouseY) * 0.5f;
        rotationY += (x - lastMouseX) * 0.5f;
        lastMouseX = x;
        lastMouseY = y;

        glutPostRedisplay();
    }
    else if (isObjectDragging) {
        float sensitivity = 0.01f;

        glm::vec3 objectPos(objectTranslationX, objectTranslationY, objectTranslationZ);

        glm::vec3 forward = glm::normalize(objectPos - cameraPos);
        glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(worldUp, forward));
        glm::vec3 up = glm::normalize(glm::cross(forward, right));

        objectTranslationX += (x - lastMouseX) * sensitivity * -right.x;
        objectTranslationZ += (x - lastMouseX) * sensitivity * -right.z;
        objectTranslationY += (y - lastMouseY) * sensitivity * -up.y;

        lastMouseX = x;
        lastMouseY = y;

        glutPostRedisplay();
    }
}

AABB calculateAABB(const std::vector<Triangle>& triangles) {
    AABB box;
    if (triangles.empty()) return box;

    box.min = triangles[0].vertices[0];
    box.max = triangles[0].vertices[0];

    for (const auto& tri : triangles) {
        for (int i = 0; i < 3; ++i) {
            box.min = glm::min(box.min, tri.vertices[i]);
            box.max = glm::max(box.max, tri.vertices[i]);
        }
    }
    return box;
}

void renderAABB(const AABB& box) {
    glLineWidth(3.0f);
    glBegin(GL_LINES);

    glm::vec3 vertices[8] = {
        {box.min.x, box.min.y, box.min.z},
        {box.max.x, box.min.y, box.min.z},
        {box.max.x, box.max.y, box.min.z},
        {box.min.x, box.max.y, box.min.z},
        {box.min.x, box.min.y, box.max.z},
        {box.max.x, box.min.y, box.max.z},
        {box.max.x, box.max.y, box.max.z},
        {box.min.x, box.max.y, box.max.z}
    };

    glVertex3fv(&vertices[0][0]); glVertex3fv(&vertices[1][0]);
    glVertex3fv(&vertices[1][0]); glVertex3fv(&vertices[2][0]);
    glVertex3fv(&vertices[2][0]); glVertex3fv(&vertices[3][0]);
    glVertex3fv(&vertices[3][0]); glVertex3fv(&vertices[0][0]);

    glVertex3fv(&vertices[4][0]); glVertex3fv(&vertices[5][0]);
    glVertex3fv(&vertices[5][0]); glVertex3fv(&vertices[6][0]);
    glVertex3fv(&vertices[6][0]); glVertex3fv(&vertices[7][0]);
    glVertex3fv(&vertices[7][0]); glVertex3fv(&vertices[4][0]);

    glVertex3fv(&vertices[0][0]); glVertex3fv(&vertices[4][0]);
    glVertex3fv(&vertices[1][0]); glVertex3fv(&vertices[5][0]);
    glVertex3fv(&vertices[2][0]); glVertex3fv(&vertices[6][0]);
    glVertex3fv(&vertices[3][0]); glVertex3fv(&vertices[7][0]);

    glEnd();
}

bool loadSTL(const std::string& filepath, std::vector<Triangle>& triangles) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open STL file: " << filepath << std::endl;
        return false;
    }

    char header[80] = "";
    file.read(header, 80);

    uint32_t triangleCount;
    file.read(reinterpret_cast<char*>(&triangleCount), sizeof(uint32_t));

    triangles.resize(triangleCount);

    for (uint32_t i = 0; i < triangleCount; ++i) {
        Triangle& tri = triangles[i];

        file.read(reinterpret_cast<char*>(&tri.normal), sizeof(glm::vec3));
        file.read(reinterpret_cast<char*>(&tri.vertices[0]), sizeof(glm::vec3));
        file.read(reinterpret_cast<char*>(&tri.vertices[1]), sizeof(glm::vec3));
        file.read(reinterpret_cast<char*>(&tri.vertices[2]), sizeof(glm::vec3));

        uint16_t attributeByteCount;
        file.read(reinterpret_cast<char*>(&attributeByteCount), sizeof(uint16_t));
    }

    file.close();
    return true;
}

void renderSTL(const std::vector<Triangle>& triangles) {
    glBegin(GL_TRIANGLES);
    for (const auto& tri : triangles) {
        glNormal3fv(&tri.normal[0]);
        for (int i = 0; i < 3; ++i) {
            glVertex3fv(&tri.vertices[i][0]);
        }
    }
    glEnd();
}

std::vector<Triangle> stlModel1;
AABB modelAABB1;
std::vector<Triangle> stlModel2;
AABB modelAABB2;

// Octree 노드 포인터
OctreeNode* octree1 = nullptr;
OctreeNode* octree2 = nullptr;

bool checkAABBCollision(const AABB& box1, const AABB& box2) {
    return (box1.min.x <= box2.max.x && box1.max.x >= box2.min.x) &&
        (box1.min.y <= box2.max.y && box1.max.y >= box2.min.y) &&
        (box1.min.z <= box2.max.z && box1.max.z >= box2.min.z);
}

void renderAABBWithColor(const AABB& box, const glm::vec3& color) {
    glColor3f(color.r, color.g, color.b);
    renderAABB(box);
}

bool renderOctreeCollision(const OctreeNode* node, const AABB& otherAABB) {
    if (!node) return false;

    bool hasCollision = checkAABBCollision(node->box, otherAABB);

    glm::vec3 color = hasCollision ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(1.0f, 1.0f, 1.0f);
    renderAABBWithColor(node->box, color);

    for (int i = 0; i < 8; ++i) {
        if (node->children[i]) {
            hasCollision |= renderOctreeCollision(node->children[i], otherAABB);
        }
    }

    return hasCollision;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(cameraPos.x, cameraPos.y, cameraPos.z,
        0.0, 0.0, 0.0,
        0.0, 1.0, 0.0);

    glRotatef(rotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotationY, 0.0f, 1.0f, 0.0f);

    glPushMatrix();
    glColor3f(0.5f, 0.5f, 0.5f);
    renderSTL(stlModel1);
    renderOctree(octree1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(objectTranslationX, objectTranslationY, objectTranslationZ);
    glColor3f(0.5f, 0.5f, 0.5f);
    renderSTL(stlModel2);

    AABB movedAABB2 = modelAABB2;
    movedAABB2.min += glm::vec3(objectTranslationX, objectTranslationY, objectTranslationZ);
    movedAABB2.max += glm::vec3(objectTranslationX, objectTranslationY, objectTranslationZ);

    renderOctree(octree2);
    glPopMatrix();

    bool collision = checkAABBCollision(modelAABB1, movedAABB2);

    if (collision)
    {
        glColor3f(1.0f, 0.0f, 0.0f);
    }
    else
    {
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    renderAABB(modelAABB1);

    if (collision)
    {
        glColor3f(1.0f, 0.0f, 0.0f);
    }
    else
    {
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    renderAABB(movedAABB2);

    glutSwapBuffers();
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)width / (double)height, 0.1, 100.0);
}

void setupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[] = { -2.0f, 2.0f, 0.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    GLfloat ambientLight[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat diffuseLight[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat specularLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}

int main(int argc, char** argv) {
    std::string filepath1 = "C:/Users/brian/OneDrive/바탕 화면/3d_bounding_box/cat.stl";
    if (!loadSTL(filepath1, stlModel1)) {
        return -1;
    }
    modelAABB1 = calculateAABB(stlModel1);

    std::string filepath2 = "C:/Users/brian/OneDrive/바탕 화면/3d_bounding_box/dog.stl";
    if (!loadSTL(filepath2, stlModel2)) {
        return -1;
    }
    modelAABB2 = calculateAABB(stlModel2);

    // Octree 생성
    octree1 = buildOctree(modelAABB1, stlModel1, 0);
    octree2 = buildOctree(modelAABB2, stlModel2, 0);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("STL Renderer");

    glEnable(GL_DEPTH_TEST);
    setupLighting();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);

    glutMainLoop();

    // Octree 삭제
    deleteOctree(octree1);
    deleteOctree(octree2);

    return 0;
}
