#pragma once

#include <RcppArmadillo.h>
#include "lambdaMax.h"

using namespace arma;
using namespace Rcpp;

template <typename T>
void regularizationPath(vec& alpha,
                        vec& lambda,
                        double& alpha_max,
                        const T& x,
                        const mat& y,
                        const rowvec& y_scale,
                        const std::string lambda_type,
                        const std::string alpha_type,
                        const double lambda_min_ratio,
                        const double q,
                        const std::string family,
                        const bool intercept)
{
  const sword n = x.n_rows;
  const uword m = y.n_cols;
  const sword n_lambda = lambda.n_elem;
  const uword path_length = alpha.n_elem;

  if (lambda_type == "gaussian" || lambda_type == "bh") {
    lambda = regspace(1, n_lambda)*q/(2*n_lambda);

    lambda.transform([](double val) {
      return Rf_qnorm5(1.0 - val, 0.0, 1.0, 1, 0);
    });

    if (lambda_type == "gaussian" && n_lambda > 1) {
      double sum_sq = 0.0;

      for (sword i = 1; i < n_lambda; ++i) {
        sum_sq += std::pow(lambda(i - 1), 2);
        double w = std::max(1.0, static_cast<double>(n - i - 1));
        lambda(i) *= std::sqrt(1.0 + sum_sq/w);
      }

      // ensure non-increasing lambda
      lambda.tail(n_lambda - lambda.index_min()).fill(min(lambda));
    }

  } else if (lambda_type == "oscar") {

    lambda = q*(regspace(n_lambda, 1) - 1) + 1;

  } else if (lambda_type == "user") {
    // standardize lambda with number of observations
    lambda *= static_cast<double>(n);
  }

  vec lambda_max = lambdaMax(x,
                             y,
                             y_scale,
                             m,
                             family,
                             intercept);

  alpha_max =
    (cumsum(sort(abs(lambda_max), "descending"))/cumsum(lambda)).max();

  if (alpha_type == "auto") {
    alpha = exp(linspace(log(alpha_max),
                         log(alpha_max*lambda_min_ratio),
                         path_length));
  }
}
