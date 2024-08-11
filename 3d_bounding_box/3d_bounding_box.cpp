#include <iostream>
#include <vector>
#include <GL/glut.h>
#include <glm/glm/glm.hpp>
#include <fstream>
#include <sstream>

float rotationX = 0.0f;
float rotationY = 0.0f;
int lastMouseX, lastMouseY;
bool isDragging = false;

struct Triangle {
    glm::vec3 normal;
    glm::vec3 vertices[3];
};

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

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

void mouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
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

OctreeNode* buildOctree(const std::vector<Triangle>& triangles, const AABB& box, int depth = 0) {
    OctreeNode* node = new OctreeNode();
    node->box = box;

    if (depth >= 2 || triangles.size() <= 1) {
        node->triangles = triangles;
        return node;
    }

    glm::vec3 center = (box.min + box.max) * 0.5f;
    std::vector<Triangle> childrenTriangles[8];

    for (const auto& tri : triangles) {
        bool assigned = false;
        for (int i = 0; i < 8; ++i) {
            glm::vec3 min = box.min;
            glm::vec3 max = center;

            if (i & 1) min.x = center.x; else max.x = center.x;
            if (i & 2) min.y = center.y; else max.y = center.y;
            if (i & 4) min.z = center.z; else max.z = center.z;

            AABB childBox{ min, max };

            bool intersects = false;
            for (int j = 0; j < 3; ++j) {
                if (tri.vertices[j].x >= min.x && tri.vertices[j].x <= max.x &&
                    tri.vertices[j].y >= min.y && tri.vertices[j].y <= max.y &&
                    tri.vertices[j].z >= min.z && tri.vertices[j].z <= max.z) {
                    intersects = true;
                }
            }

            if (intersects) {
                childrenTriangles[i].push_back(tri);
                assigned = true;
            }
        }

        if (!assigned) {
            node->triangles.push_back(tri);
        }
    }

    for (int i = 0; i < 8; ++i) {
        if (!childrenTriangles[i].empty()) {
            glm::vec3 min = box.min;
            glm::vec3 max = center;

            if (i & 1) min.x = center.x; else max.x = center.x;
            if (i & 2) min.y = center.y; else max.y = center.y;
            if (i & 4) min.z = center.z; else max.z = center.z;

            node->children[i] = buildOctree(childrenTriangles[i], { min, max }, depth + 1);
        }
    }

    return node;
}


void renderOctree(const OctreeNode* node, int depth = 0) {
    if (!node) return;

    // 깊이에 따라 색상 변경
    if (depth == 0)
        glColor3f(1.0f, 0.0f, 0.0f); // 레드
    else if (depth == 1)
        glColor3f(0.0f, 1.0f, 0.0f); // 그린
    else if (depth == 2)
        glColor3f(0.0f, 0.0f, 1.0f); // 블루

    // 깊이에 따라 선 두께를 다르게 설정
    glLineWidth(1.0f + depth);
    renderAABB(node->box);

    if (depth < 2) {  // 2단계까지만 자식 노드 렌더링
        for (int i = 0; i < 8; ++i) {
            renderOctree(node->children[i], depth + 1);
        }
    }
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

std::vector<Triangle> stlModel;
AABB modelAABB;
OctreeNode* octreeRoot = nullptr;
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.5, 0.0, 0.0, 
        0.0, 0.0, 0.0,   
        0.0, 1.0, 0.0);  

    glRotatef(rotationX, 1.0f, 0.0f, 0.0f); // X축 회전
    glRotatef(rotationY, 0.0f, 1.0f, 0.0f); // Y축 회전

    renderSTL(stlModel);

    // Octree 렌더링
    renderOctree(octreeRoot);

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
}

int main(int argc, char** argv) {
    std::string filepath = "C:/Users/brian/OneDrive/바탕 화면/3d_bounding_box/cat.stl";
    if (!loadSTL(filepath, stlModel)) {
        return -1;
    }
    // AABB 계산
    modelAABB = calculateAABB(stlModel);
    // Octree 생성
    octreeRoot = buildOctree(stlModel, modelAABB);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("STL Renderer");

    glEnable(GL_DEPTH_TEST);
    setupLighting(); 

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouseButton);      // 마우스 버튼 콜백
    glutMotionFunc(mouseMotion);     // 마우스 이동 콜백

    glutMainLoop();
    // Octree 메모리 해제
    delete octreeRoot;
    return 0;
}
