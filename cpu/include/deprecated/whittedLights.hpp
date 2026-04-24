
// class PointLight : public Light {
// public:
//   PointLight(const gl::vec3 &position, const gl::vec3 &color,
//              float intensity = 1.0f) {
//     this->type = LightType::POINT_LIGHT;
//     this->position = position;
//     this->color = color;
//     this->intensity = intensity;
//   };

//   ~PointLight() = default;
// };

// class DirectionalLight : public Light {
// public:
//   DirectionalLight(const gl::vec3 &color, const gl::vec3 &direction,
//                    float intensity) {
//     this->color = color;
//     this->defaultFront = direction;
//     this->intensity = intensity;
//     this->position = gl::vec3(0.0f, 0.0f, 0.0f);
//     this->type = LightType::DIRECTIONAL_LIGHT;
//   };

//   ~DirectionalLight() = default;
// };

// class SpotLight : public Light {
// public:
//   SpotLight(const gl::vec3 &direction, const gl::vec3 &position,
//             const gl::vec3 &color, float intensity,
//             float cutoff_angle = gl::to_radian(180.0f),
//             float dropoff_rate = 1.0f) {
//     this->defaultFront = direction;
//     this->position = position;
//     this->color = color;
//     this->intensity = intensity;
//     this->cutoff_angle = cutoff_angle;
//     this->dropoff_rate = dropoff_rate;
//     this->type = LightType::SPOT_LIGHT;
//   };

//   ~SpotLight() = default;
//   float cutoff_angle = gl::to_radian(180.0f);
//   float dropoff_rate = 1.0f;
// };