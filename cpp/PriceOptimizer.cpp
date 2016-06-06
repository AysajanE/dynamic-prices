#include <vector>
#include <utility>
#include <algorithm>
#include <cmath>


int rank(double price, std::vector<double> competitor_prices) {
  int rank = competitor_prices.size();
  for (int i = 0; i < competitor_prices.size(); i++) {
    if (price < competitor_prices[i]) {
      rank = rank - 1;
    }
  }
  return rank;
}

unsigned int factorial(unsigned int n) {
  return (n == 0) ? 1 : factorial(n - 1);
}

double poisson_pdf(unsigned int i, double mu) {
  return std::pow(mu, i) / factorial(i) * std:: exp(-mu);
}

unsigned int poisson_ppf(double mu, double q) {
  double sum = 0;
  unsigned int i = 0;
  while(sum < q) {
    sum += poisson_pdf(i, mu);
    i++;
  }
  return i;
}

double predict_logistic_regression(std::vector<double> x, std::vector<double> coeff) {
  double sum = 0;
  for (int i = 0; i < std::min(x.size(), coeff.size()); i++) {
    sum += x[i] * coeff[i];
  }
  return std::exp(sum) / (1 + std::exp(sum));
}


class PriceOptimizer {
  public:
    PriceOptimizer(int, int);
    int T;
    int N;
    int M = 10;
    double L = 0.1;
    double Z = 0.5;
    double delta = 0.99;
    std::vector<double> price_range = {};
    std::vector<double> competitor_prices = {};
    std::vector<double> sales_model_coeff = {};

    std::pair<double, double> run(int, int);

  private:
    std::vector< std::pair<double, double> > cache;
    static std::pair<double, double> cache_default;

    double _V(double, int, int);
    std::pair<double, double> V_impl(int, int);
    std::pair<double, double> V(int, int);
    double sales_model(double, int);
};

std::pair<double, double> PriceOptimizer::cache_default = 
  std::make_pair<double, double>(-100000, -100000);

PriceOptimizer::PriceOptimizer(int T, int N) {
  this->T = T;
  this->N = N;
  cache = std::vector< std::pair<double, double> >((T + 1) * (N + 1));
  std::fill(cache.begin(), cache.end(), cache_default);
}

double PriceOptimizer::_V(double price, int t, int n) {
  double p = sales_model(price, t);
  double sum = 0;
  int i_max = poisson_ppf(M * p, 0.9999);
  for (int i = 0; i <= i_max; i++) {
    if (i > n) {
      break;
    }

    double pi = poisson_pdf(i, M * p);
    double today_profit = std::min(n, i) * price;
    double holding_costs = n * L;
    double V_future = std::get<1>(V(t + 1, std::max(0, n - i)));
    double exp_future_profits = delta * V_future;
    sum += pi * (today_profit - holding_costs + exp_future_profits);
  }
  return sum;
}

std::pair<double, double> PriceOptimizer::V_impl(int t, int n) {
  if (t >= T) {
    return std::make_pair(0, n * Z);
  }
  if (n <= 0) {
    return std::make_pair(0, 0);
  }

  std::pair<double, double> price_opt_pair;

  for (double price: price_range) {
    double v = _V(price, t, n);
    if (v > std::get<1>(price_opt_pair)) { 
      price_opt_pair = std::make_pair(price, v);
    }
  }
  return price_opt_pair;
}

std::pair<double, double> PriceOptimizer::V(int t, int n) {
  if (cache[t + T * n] != cache_default) {
    return cache[t + T * n];
  }

  std::pair<double, double> price_pair = V_impl(t, n);
  cache[t + T * n] = price_pair;
  
  // printf("%d %d %f %f\n", t, n, std::get<0>(price_pair), std::get<1>(price_pair));
  return price_pair;
}

std::pair<double, double> PriceOptimizer::run(int t, int n) {
  return V(t, n);
}

double PriceOptimizer::sales_model(double price, int t) {
  std::vector<double> x = { 
    price, 
    competitor_prices[0],
    competitor_prices[1],
    competitor_prices[2],
    competitor_prices[3],
    competitor_prices[4],
    double(rank(price, competitor_prices))
  };
  return predict_logistic_regression(x, sales_model_coeff);
}