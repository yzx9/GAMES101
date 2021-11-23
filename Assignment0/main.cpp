#include <cmath>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <iostream>

Eigen::Vector4d Transform(Eigen::Vector4d point) {
    auto affine = Eigen::Translation3d(2, 1, 0) *
                  Eigen::AngleAxisd(EIGEN_PI / 4, Eigen::Vector3d::UnitZ());
    return affine * point;
}

int main()
{
    std::cout << Transform({ 2, 1, 0, 1 }) << std::endl;
    return 0;
}
