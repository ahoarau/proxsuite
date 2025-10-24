#include <Eigen/Core>
#include <iostream>
#include <faer.hpp>

int main() {
    using T = quad::f128;
    using Mat = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;

    Mat A(10, 10);
    A.setRandom();
    A = A * A.transpose();

    using namespace faer::linalg::cholesky;

    Mat L = A;
    llt::factor::in_place(faer::from_eigen(L));
    L.triangularView<Eigen::StrictlyUpper>().setZero();
    std::cout << (A - L * L.transpose()).norm() << std::endl;
}
