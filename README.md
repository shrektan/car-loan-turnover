# A simple car loan model using Rcpp

A friend of mine ask me that he want to do a car loan business. 

- He has 10 million cash. 
- He will use this cash to lend to a car buyer. 
- The lending amount on average is 100,000.

There're two plans:

1. The car seller will return 130,000 on day 12. The 30,000 extra money is the profit for selling the car.
1. The car seller will return 90,000 on day 3 with the additional 40,000 on day 35.

So, which plan is preferred?

There's no straight answer for this question. It depends on at least two conditions:

1. How many cars is he going to sell on average?
1. The maximum money he can use is only the initial 10 million or the profit that's gained in the business could be added into the fund pool?

In order to answer this question carefully, I draft this tiny model to illustrate that cpp is good for modelling certain problems, like this. And it's very convinient to combine the power of R using Rcpp.

