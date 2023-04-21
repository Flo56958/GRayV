#include "camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

#define _USE_MATH_DEFINES
#include <math.h>

static glm::mat4 getProjectionMatrix(float left, float right, float top, float bottom, float n, float f) {
    glm::mat4 proj = glm::mat4(0);
    proj[0][0] = (2.f * n) / (right - left);
    proj[1][1] = (2.f * n) / (top - bottom);
    proj[2][0] = (right + left) / (right - (left));
    proj[2][1] = (bottom + top) / (top - bottom);
    proj[2][2] = -(f + n) / (f - n);
    proj[2][3] = -1.f;
    proj[3][2] = (-2 * f * n) / (f - n);
    return proj;
}

Camera::Camera() :
    pos(0, 0, 0), dir(1, 0, 0), up(0, 1, 0),
    fov_degree(70),
    near(0.01f), far(1000),
    left(-100), right(100), bottom(-100), top(100),
    perspective(true), skewed(false)
{
    update();
}

Camera::~Camera() {

}


void Camera::update() {
    dir = glm::normalize(dir);
    up = glm::normalize(up);
    view = glm::lookAt(pos, pos + dir, up);
    view_normal = glm::transpose(glm::inverse(view));
    proj = perspective ? (skewed ? getProjectionMatrix(left, right, top, bottom, near, far)
        : glm::perspective(fov_degree * float(M_PI / 180), aspect_ratio, near, far))
        : glm::ortho(left, right, bottom, top, near, far);
}

void Camera::move_forward(float by) { pos += by * dir; }
void Camera::move_backward(float by) { pos -= by * dir; }
void Camera::move_left(float by) { pos -= by * glm::cross(dir, up); }
void Camera::move_right(float by) { pos += by * glm::cross(dir, up); }
void Camera::move_up(float by) { pos += by * glm::normalize(glm::cross(glm::cross(dir, up), dir)); }
void Camera::move_down(float by) { pos -= by * glm::normalize(glm::cross(glm::cross(dir, up), dir)); }

void Camera::yaw(float angle) { dir = glm::normalize(glm::rotate(dir, angle * float(M_PI) / 180.f, up)); }
void Camera::pitch(float angle) {
    dir = glm::normalize(glm::rotate(dir, angle * float(M_PI) / 180.f, glm::normalize(glm::cross(dir, up))));
    up = glm::normalize(glm::cross(glm::cross(dir, up), dir));
}
void Camera::roll(float angle) { up = glm::normalize(glm::rotate(up, angle * float(M_PI) / 180.f, dir)); }