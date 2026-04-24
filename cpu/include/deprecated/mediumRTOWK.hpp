
// class ConstantMedium : public Hittable
// {
// public:
//   ConstantMedium(std::shared_ptr<Hittable> boundary, float density, const ColorVariant &color)
//       : boundary(boundary), density(density),
//         phase(std::make_shared<Isotropic>(color))
//   {
//     this->objtype = ObjType::MEDIUM_OBJ;
//   };

//   bool intersect(const Ray &ray, HitRecord &hit_record, float tmin = 0.0001,
//                  float tmax = 10000.f) const override;

//   AABB getAABB(float t0, float t1) override
//   {
//     return boundary->getAABB(t0, t1);
//   };

//   std::shared_ptr<Hittable> boundary;
//   float density;
//   std::shared_ptr<Material> phase;
// };

// inline bool ConstantMedium::intersect(const Ray &ray, HitRecord &hit_record,
//                                       float tmin, float tmax) const
// {
//   HitRecord rec_1, rec_2;
//   if (!boundary->intersect(ray, rec_1, tmin, tmax))
//     return false;
//   if (!boundary->intersect(ray, rec_2, rec_1.t + 0.0001, tmax))
//     return false;

//   rec_1.t = std::max(rec_1.t, tmin);
//   rec_2.t = std::min(rec_2.t, tmax);

//   if (rec_1.t >= rec_2.t)
//     return false;

//   rec_1.t = std::max(rec_1.t, 0.0001f);

//   const float boundary_distance =
//       (rec_2.t - rec_1.t) * ray.getDirection().length();
//   const float hit_distance = -(1.f / density) * std::log(gl::rand_num());

//   if (hit_distance > boundary_distance)
//     return false;

//   if (hit_distance < boundary_distance)
//   {
//     hit_record.t = rec_1.t + hit_distance / ray.getDirection().length();
//     hit_record.position = ray.getOrigin() + hit_record.t * ray.getDirection();
//     hit_record.normal = gl::vec3(1, 0, 0);
//     hit_record.material = phase;
//     return true;
//   }

//   return false;
// }