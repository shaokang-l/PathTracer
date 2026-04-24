#include "./light.hpp"
// Lights _get_lights_from_io(const LightIO *io)
// {
//     std::vector<PointLight> pointLights;
//     std::vector<DirectionalLight> directionalLights;

//     if (io == nullptr)
//     {
//         std::cout << "No light is found" << std::endl;
//         return {};
//     }

//     while (io != nullptr)
//     {
//         switch (io->type)
//         {
//         case LightType::POINT_LIGHT:
//             pointLights.push_back(PointLight(io->position, io->color, 1.0f));
//             break;
//         case LightType::DIRECTIONAL_LIGHT:
//             directionalLights.push_back(DirectionalLight(io->color, io->direction, 1.0f));
//             break;
//         case LightType::SPOT_LIGHT:
//             // lights.push_back(std::make_unique<SpotLight>(io->direction, io->position, io->color, 1.0f));
//             break;
//         default:
//             break;
//         }

//         io = io->next;
//     };

//     return {pointLights, directionalLights};
// };