#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>

// ----------------------------------------------------
// Camera

class Camera {
public:
    Camera();
    virtual ~Camera();

    void update();

    // move
    void move_forward(float by);
    void move_backward(float by);
    void move_left(float by);
    void move_right(float by);
    void move_up(float by);
    void move_down(float by);

    // rotate
    void yaw(float angle);
    void pitch(float angle);
    void roll(float angle);

    // data
    glm::vec3 pos, dir, up;             // camera coordinate system
    float fov_degree, near, far;        // perspective projection
    float left, right, bottom, top;     // orthographic projection or skewed frustum
    bool perspective;                   // switch between perspective and orthographic (default: perspective)
    bool skewed;                        // switcg between normal perspective and skewed frustum (default: normal)
    glm::mat4 view, view_normal, proj;  // camera matrices (computed via a call update())
    float aspect_ratio;
};