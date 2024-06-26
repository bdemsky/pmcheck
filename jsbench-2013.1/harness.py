#!/usr/bin/env python
# Copyright (C) 2011, 2012 Purdue University
# Written by Gregor Richards
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import math
import os
import re
import sys

benchmarks = [
    "amazon/chrome", "amazon/chrome-win", "amazon/firefox",
    "amazon/firefox-win", "amazon/safari",
    "facebook/chrome", "facebook/chrome-win", "facebook/firefox",
    "facebook/firefox-win", "facebook/safari",
    "google/chrome", "google/chrome-win", "google/firefox",
    "google/firefox-win", "google/safari",
    "twitter/chrome", "twitter/chrome-win", "twitter/firefox",
    "twitter/firefox-win", "twitter/safari",
    "yahoo/chrome", "yahoo/chrome-win", "yahoo/firefox",
    "yahoo/firefox-win", "yahoo/safari",
]

modes = {
    "*": ["urem"],
    "amazon/firefox": ["urm"],
    "amazon/firefox-win": ["urm"],
    "google/firefox": ["uem"],
    "twitter/chrome-win": ["rem"]
};

'''
benchmarks = ["amazon/chrome", "amazon/firefox", "amazon/safari",
              "facebook/chrome", "facebook/firefox", "facebook/safari",
              "google/chrome", "google/firefox", "google/safari",
              "twitter/chrome", "twitter/firefox", "twitter/safari",
              "yahoo/chrome", "yahoo/firefox", "yahoo/safari"]
modes = {
    "*": ["urem"],
    "amazon/firefox": ["urm"],
    "google/firefox": ["uem"]
}'''

#runcount = 25

if len(sys.argv) != 3:
    print "Use: python harness.py <JS executable> <number of runs>"
    exit(1)
js = sys.argv[1]
runcount = int(sys.argv[2])

keepruns = 20
keepfrom = runcount - keepruns

# standard t-distribution for normally distributed samples
tDistribution = [0, 0, 12.71, 4.30, 3.18, 2.78, 2.57, 2.45, 2.36, 2.31, 2.26,
2.23, 2.20, 2.18, 2.16, 2.14, 2.13, 2.12, 2.11, 2.10, 2.09, 2.09, 2.08, 2.07,
2.07, 2.06, 2.06, 2.06, 2.05, 2.05, 2.05, 2.04, 2.04, 2.04, 2.03, 2.03, 2.03,
2.03, 2.03, 2.02, 2.02, 2.02, 2.02, 2.02, 2.02, 2.02, 2.01, 2.01, 2.01, 2.01,
2.01, 2.01, 2.01, 2.01, 2.01, 2.00, 2.00, 2.00, 2.00, 2.00, 2.00, 2.00, 2.00,
2.00, 2.00, 2.00, 2.00, 2.00, 2.00, 2.00, 1.99, 1.99, 1.99, 1.99, 1.99, 1.99,
1.99, 1.99, 1.99, 1.99, 1.99, 1.99, 1.99, 1.99, 1.99, 1.99, 1.99, 1.99, 1.99,
1.99, 1.99, 1.99, 1.99, 1.99, 1.99, 1.99, 1.99, 1.98, 1.98, 1.98, 1.98, 1.98,
1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98,
1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98,
1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98,
1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.98,
1.98, 1.98, 1.98, 1.98, 1.98, 1.98, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97,
1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.97, 1.96]

def tDist(n):
    if (n >= len(tDistribution)):
        return tDistribution[-1]
    return tDistribution[n]

results = {}

for benchmark in benchmarks:
    results[benchmark] = {}

    bmodes = modes["*"]
    if benchmark in modes:
        bmodes = modes[benchmark]

    for mode in bmodes:
        results[benchmark][mode] = []

        for runno in range(runcount):
            # Now run it and get the results
            print(benchmark + " " + mode + " " + str(runno))
            res = os.popen(js + " " + benchmark + "/" + mode + ".js").read()
            time = float(re.findall("Time: ([0-9]*)ms", res)[0])
#            time = float(re.match("Time: ([0-9]*)ms", res).group(1))

            if runno >= keepfrom:
                results[benchmark][mode].append(time)

# Collect the totals
sresults = {}
totals = {
    "mean": 1,
    "stddev": 1,
    "sem": 1,
    "ci": 1,
    "runs": 0
}

power = 1.0 / len(benchmarks)

for benchmark in benchmarks:
    sresults[benchmark] = {}

    bmodes = modes["*"]
    if benchmark in modes:
        bmodes = modes[benchmark]

    for mode in bmodes:
        sresults[benchmark][mode] = sresult = {}
        result = results[benchmark][mode]
        totals["runs"] = totals["runs"] + 1

        sresult["mode"] = mode

        mean = sresult["mean"] = float(sum(result)) / len(result)
        stddev = sresult["stddev"] = math.sqrt(
            sum(
                map(lambda e: math.pow(e - mean, 2), result)
            ) / float((len(result) - 1))
        )

        sm = sresult["sm"] = stddev / float(mean)
        sem = sresult["sem"] = stddev / float(math.sqrt(len(result)))
        semm = sresult["semm"] = sem / float(mean)
        ci = sresult["ci"] = tDist(len(result)) * sem
        cim = sresult["cim"] = ci / float(mean)

        totals["mean"] *= math.pow(mean, power)

        if (stddev == 0):
            totals["stddev"] *= 1
            print("stddev is 0")
        else:
            totals["stddev"] *= math.pow(stddev, power)
        totals["sem"] *= math.pow(sem, power)
        totals["ci"] *= math.pow(ci, power)

#power = 1.0 / totals["runs"]
#totals["mean"] = math.pow(totals["mean"], power)
#totals["stddev"] = math.pow(totals["stddev"], power)
#totals["sem"] = math.pow(totals["sem"], power)
#totals["ci"] = math.pow(totals["ci"], power)

totals["sm"] = totals["stddev"] / float(totals["mean"])
totals["semm"] = totals["sem"] / float(totals["mean"])
totals["cim"] = totals["ci"] / float(totals["mean"])

totals["sm"] *= 100
totals["semm"] *= 100
totals["cim"] *= 100

print "Final results:"
print "  %(mean)fms +- %(cim)f%% (lower is better)" % totals
print "  Standard deviation = %(sm)f%% of mean" % totals
print "  Standard error = %(semm)f%% of mean" % totals
print "  %(runs)d runs" % {"runs": runcount}
print ""

print "Result breakdown:"
for benchmark in benchmarks:
    print "  %(benchmark)s:" % {"benchmark": benchmark}
    
    bmodes = modes["*"]
    if benchmark in modes:
        bmodes = modes[benchmark]

    for mode in bmodes:
        print "  %(mode)s: %(mean)fms +- %(cim)f%% (stddev=%(sm)f%%, stderr=%(semm)f%%)" % sresults[benchmark][mode]

print ""

print "Raw results:"
for benchmark in benchmarks:
    print "  %(benchmark)s:" % {"benchmark": benchmark}

    bmodes = modes["*"]
    if benchmark in modes:
        bmodes = modes[benchmark]

    for mode in bmodes:
        print "    %(mode)s: %(results)s" % {
            "mode": mode,
            "results": results[benchmark][mode]
        }
