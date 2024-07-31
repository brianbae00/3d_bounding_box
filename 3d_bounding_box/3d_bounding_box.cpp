#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

struct Triangle {
    glm::vec3 normal;
    glm::vec3 vertex1, vertex2, vertex3;
};

std::vector<Triangle> loadSTL(const std::string& filepath) {
    std::vector<Triangle> triangles;
    std::ifstream file(filepath, std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return triangles;
    }

    char header[80] = "";
    file.read(header, 80);

    unsigned int numTriangles = 0;
    file.read(reinterpret_cast<char*>(&numTriangles), sizeof(unsigned int));

    for (unsigned int i = 0; i < numTriangles; ++i) {
        Triangle triangle;
        file.read(reinterpret_cast<char*>(&triangle.normal), sizeof(glm::vec3));
        file.read(reinterpret_cast<char*>(&triangle.vertex1), sizeof(glm::vec3));
        file.read(reinterpret_cast<char*>(&triangle.vertex2), sizeof(glm::vec3));
        file.read(reinterpret_cast<char*>(&triangle.vertex3), sizeof(glm::vec3));
        triangles.push_back(triangle);
        file.ignore(2);
    }

    file.close();
    return triangles;
}

void drawOOBB(const std::vector<Triangle>& triangles) {
    if (triangles.empty()) return;

    std::vector<glm::vec3> vertices;
    for (const auto& triangle : triangles) {
        vertices.push_back(triangle.vertex1);
        vertices.push_back(triangle.vertex2);
        vertices.push_back(triangle.vertex3);
    }

    glm::vec3 min = vertices[0];
    glm::vec3 max = vertices[0];
    for (const auto& vertex : vertices) {
        min = glm::min(min, vertex);
        max = glm::max(max, vertex);
    }

    glm::vec3 center = (min + max) * 0.5f;
    glm::vec3 size = max - min;

    glPushMatrix();
    glTranslatef(center.x, center.y, center.z);
    glScalef(size.x, size.y, size.z);

    glBegin(GL_LINES);
    glVertex3f(-0.5f, -0.5f, -0.5f); glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, -0.5f); glVertex3f(0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f); glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f, -0.5f); glVertex3f(-0.5f, -0.5f, -0.5f);

    glVertex3f(-0.5f, -0.5f, 0.5f); glVertex3f(0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f); glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f); glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f); glVertex3f(-0.5f, -0.5f, 0.5f);

    glVertex3f(-0.5f, -0.5f, -0.5f); glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, -0.5f, -0.5f); glVertex3f(0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f); glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, -0.5f); glVertex3f(-0.5f, 0.5f, 0.5f);
    glEnd();

    glPopMatrix();
}

std::vector<Triangle> model;
glm::vec3 center;

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // 카메라 위치를 모델의 중심에 기반하여 설정
    gluLookAt(center.x + 3.0f, center.y + 3.0f, center.z + 3.0f,  // 카메라 위치
        center.x, center.y, center.z,                      // 모델의 중심
        0.0f, 1.0f, 0.0f);                                // 업 벡터

    drawOOBB(model);

    glutSwapBuffers();
}

void initOpenGL(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("STL OOBB Example");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glColor3f(1.0f, 1.0f, 1.0f);

    glutDisplayFunc(display);
    glutMainLoop();
}

int main(int argc, char** argv) {

    model = loadSTL("C:/Users/brian/OneDrive/바탕 화면/3d_bounding_box/cat.stl");


    // 모델의 중심을 계산
    glm::vec3 min = model[0].vertex1;
    glm::vec3 max = model[0].vertex1;
    for (const auto& triangle : model) {
        min = glm::min(min, triangle.vertex1);
        min = glm::min(min, triangle.vertex2);
        min = glm::min(min, triangle.vertex3);
        max = glm::max(max, triangle.vertex1);
        max = glm::max(max, triangle.vertex2);
        max = glm::max(max, triangle.vertex3);
    }
    center = (min + max) * 0.5f;

    initOpenGL(argc, argv);
    return 0;
}
