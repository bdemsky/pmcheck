#include <chrono>
#include <iostream>
#include <thread>
#include <atomic>
#include <cstdlib>

#include "gdax-orderbook.hpp"

#define FACTOR 500
#define HIST_SIZE 40

std::atomic<bool> stop;

void printBestBidAndOffer(GDAXOrderBook & book)
{
    std::cout << "current best bid: " << book.bids.begin()->second << " @ $"
                               << book.bids.begin()->first/100.0 << " ; ";
    std::cout << "current best offer: " << book.offers.begin()->second << " @ $"
                                 << book.offers.begin()->first/100.0 << " ; "
                                 << std::endl;
}

int main(int argc, char* argv[]) {
    GDAXOrderBook book("ETH-USD");

//    printBestBidAndOffer(book);

    size_t secondsToSleep = 30;
    if (argc == 2) {
        secondsToSleep = atoi(argv[1]);
    }
/*
    std::cout << "waiting " << secondsToSleep << " seconds for the market to "
        "shift" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(secondsToSleep));

    printBestBidAndOffer(book);
*/

    size_t histogram[HIST_SIZE];
    for ( size_t i = 0; i < HIST_SIZE; i++ )
        histogram[i] = 0;

    size_t numThreads = 5;
    std::cout << "running for " << secondsToSleep << " seconds, with " <<
        numThreads << " threads constantly iterating over the whole order "
        "book." << std::endl;
    bool keepIterating = true;
    stop.store(false);
    {
        std::vector<std::future<void>> futures;
        for (size_t i = 0 ; i < numThreads ; ++i)
        {
            futures.emplace_back(
                std::async(std::launch::async,
                           [&book, &keepIterating, &histogram] ()
            {
                GDAXOrderBook::ensureThreadAttached();

                GDAXOrderBook::bids_map_t::iterator bidIter;
                GDAXOrderBook::offers_map_t::iterator offerIter;
                std::chrono::steady_clock::time_point start, finish;

                while(keepIterating == true)
                {
                    start = std::chrono::steady_clock::now();

                    bidIter = book.bids.begin();
                    while(bidIter != book.bids.end()) { ++bidIter; }

                    offerIter = book.offers.begin();
                    while(offerIter != book.offers.end()) { ++offerIter; }

                    finish = std::chrono::steady_clock::now();

                    int index =
                        static_cast<size_t>(
                            std::chrono::duration<double, std::milli>(
                                std::chrono::steady_clock::now() - start
                            ).count()/FACTOR
                        );

                    if (!stop.load()) {
                        if (index > HIST_SIZE)
                            fprintf(stderr, "index overflow!\n");
                        else
                            histogram[index]++;
                    }
                }
            }));
        }

        std::this_thread::sleep_for(std::chrono::seconds(secondsToSleep));

        keepIterating = false;
        stop.store(true);
    }

    // find the largest histogram bucket so we can determine the scale factor
    // for all of the buckets
    double scaleFactor =
        [&histogram]() {
            size_t countOfBiggestBucket = 0;
            for ( size_t & i : histogram )
            {
          countOfBiggestBucket = std::max(i, countOfBiggestBucket);
            }
            return (double)countOfBiggestBucket;
        }()/68.0; // 80 column display, minus chars used for row headers, =68
    std::cout << "histogram of times to iterate over the whole book:" << std::endl;
    for ( int i=0 ; i < HIST_SIZE ; ++i )
    {
        std::cout
            << std::right << std::setw(4) << std::setfill(' ') << i*FACTOR / 1000.0
            << "-"
            << std::right << std::setw(4) << std::setfill(' ') << (i+1)*FACTOR / 1000.0 - 0.01
            << " s: ";
        std::cout << histogram[i] << "\n";
/*
        for ( int j=0 ; j<histogram[i]/scaleFactor ; ++j )
        {
            std::cout << "*";
        }
        std::cout << std::endl;
*/
    }
}
