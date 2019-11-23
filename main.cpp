#include <Rcpp.h>
#include <vector>
#include <map>
// [[Rcpp::plugins(cpp14)]]
using namespace Rcpp;

const std::map<int, double> loan_cfs1 {
  {12, 13}
};

const std::map<int, double> loan_cfs2 {
  {3, 9}, {35, 4}
};

struct Loan
{
  Loan() = default;
  Loan(int _reg_day, const std::map<int, double>& _cfs)
    : reg_day(_reg_day), cfs(_cfs) {}
  const int reg_day {0};
  static const double principal;
  std::map<int, double> cfs;
  double cf(int now) const
  {
    const int days = now - reg_day;
    if (cfs.count(days)) {
      return cfs.at(days);
    } else {
      return 0.0;
    }
  }
  // doesn't include the day in now
  double future_cf(int now) const
  {
    double out = 0.0;
    for (auto& pair : cfs) {
      if (pair.first + reg_day > now) {
        out += pair.second;
      }
    }
    return out;
  }
  // doesn't include the day in now
  double future_principal(int now) const
  {
    double cf_received = 0.0;
    for (auto& pair : cfs) {
      if (pair.first + reg_day <= now) {
        cf_received += pair.second;
      }
    }
    return std::max(principal - cf_received, 0.0);
  }
  bool expired(int now) const
  {
    return future_cf(now) == 0.0;
  }
};

const double Loan::principal = 10.0;

struct Record
{
  Record() = default;
  explicit Record(int _day, double _money) : day(_day + 1), money0(_money) { }
  int day = 0;
  double money0 = 0.0;
  int new_loan_num = 0;
  double new_loan_amt = 0.0;
  double cf_received = 0.0;
  double money = 0.0;
  int loan_num = 0;
  int loan_expired_num = 0;
  int loan_unexpired_num = 0;
  double loan_principal = 0.0;
  double loan_future_cf = 0.0;
};

struct Loaner
{
  double money {1000.0};
  const double init_money {money};
  int now {0};
  std::vector<Loan> loans;
  mutable std::vector<Record> records;
  mutable Record recording = Record(now, money);
  void lend(int qty, const std::map<int, double>& cfs, bool only_init_money = true)
  {
    for (int i = 0; i < qty; ++i) {
      const bool flag1 = only_init_money && future_principal() > init_money;
      const bool flag2 = Loan::principal > money;
      if (flag1 || flag2) {
        // do nothing
      } else {
        lend(cfs);
      }
    }
  }
  void lend(const std::map<int, double>& cfs)
  {
    if (money >= Loan::principal) {
      recording.new_loan_amt += Loan::principal;
      ++recording.new_loan_num;
      money -= Loan::principal;
      loans.emplace_back(now, cfs);
    } else {
      Rcpp::stop("can't lend: only have %0.2f but need %0.2f", money, Loan::principal);
    }
  }
  void timepass()
  {
    for (const auto& loan : loans) {
      const double cf = loan.cf(now);
      recording.cf_received += cf;
      money += cf;
    }
    recording.money = money;
    recording.loan_expired_num = expired_loan_num();
    recording.loan_num = loan_num();
    recording.loan_future_cf = future_cf();
    recording.loan_unexpired_num = unexpired_loan_num();
    recording.loan_principal = future_principal();
    ++now;
    record();
  }
  void record() const
  {
    records.emplace_back(recording);
    recording = Record(now, money);
  }
  double future_cf() const
  {
    double out {0.0};
    for (const auto& loan : loans) {
      out += loan.future_cf(now);
    }
    return out;
  }
  double future_principal() const
  {
    double out {0.0};
    for (const auto& loan : loans) {
      out += loan.future_principal(now);
    }
    return out;
  }
  int loan_num() const
  {
    return int(loans.size());
  }
  int expired_loan_num() const 
  {
    int out = 0;
    for (const auto& loan : loans) {
      if (loan.expired(now)) 
        ++out;
    }
    return out;
  }
  int unexpired_loan_num() const
  {
    return loan_num() - expired_loan_num();
  }
};

DataFrame export_tbl(const std::vector<Record>& records)
{
  const int n = records.size();
  IntegerVector day(n);
  DoubleVector money0(n);
  IntegerVector new_loan_num(n);
  DoubleVector new_loan_amt(n);
  DoubleVector cf_received(n);
  DoubleVector money(n);
  IntegerVector loan_num(n);
  IntegerVector loan_expired_num(n);
  IntegerVector loan_unexpired_num(n);
  DoubleVector loan_principal(n);
  DoubleVector loan_future_cf(n);
  for (int i = 0; i < n; ++i) {
    day[i] = records[i].day;
    money0[i] = records[i].money0;
    new_loan_num[i] = records[i].new_loan_num;
    new_loan_amt[i] = records[i].new_loan_amt;
    cf_received[i] = records[i].cf_received;
    money[i] = records[i].money;
    loan_num[i] = records[i].loan_num;
    loan_expired_num[i] = records[i].loan_expired_num;
    loan_principal[i] = records[i].loan_principal;
    loan_future_cf[i] = records[i].loan_future_cf;
  }
  return DataFrame::create(
    _["day"] = day,
    _["money0"] = money0,
    _["new_loan_num"] = new_loan_num,
    _["new_loan_amt"] = new_loan_amt,
    _["cf_received"] = cf_received,
    _["money"] = money,
    _["loan_num"] = loan_num,
    _["loan_expired_num"] = loan_expired_num,
    _["loan_principal"] = loan_principal,
    _["loan_future_cf"] = loan_future_cf
  );
}

// [[Rcpp::export]]
DataFrame cal(int type, const int max_lend, const bool only_init_money)
{
  if (type != 1 && type != 2)
    stop("type must be one of 1 and 2 but input is %d", type);
  const std::map<int, double> cfs = type == 1 ? loan_cfs1 : loan_cfs2;
  // strategy is to try to lend max 10 loans every day, for one year
  const int days = 365;
  Loaner loaners;
  for (int i = 0; i < days; ++i)
  {
    loaners.lend(max_lend, cfs, only_init_money);
    loaners.timepass();
  }
  return export_tbl(loaners.records);
}

/*** R
max_lend <- 50
rbind(
  tail(cal(1, max_lend, F), 1), 
  tail(cal(2, max_lend, F), 1)
)
*/
