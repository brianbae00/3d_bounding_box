#include <iostream>
#include <vector>
#include <GL/glut.h>
#include <glm/glm/glm.hpp>
#include <fstream>
#include <sstream>

struct Triangle {
    glm::vec3 normal;
    glm::vec3 vertices[3];
};

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

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

std::vector<Triangle> stlModel;
AABB modelAABB;
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.5, 0.0, 0.0, 
        0.0, 0.0, 0.0,   
        0.0, 1.0, 0.0);  

    renderSTL(stlModel);

    // AABB 렌더링
    glColor3f(1.0f, 0.0f, 0.0f); // AABB를 빨간색으로 렌더링
    renderAABB(modelAABB);

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

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("STL Renderer");

    glEnable(GL_DEPTH_TEST);
    setupLighting(); 

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

    glutMainLoop();

    return 0;
}
