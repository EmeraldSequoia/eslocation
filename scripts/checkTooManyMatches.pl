#!/usr/bin/perl -w

use strict;

use Config;

$Config{useithreads} or die "Recompile Perl with threads to use this program.\n";

use threads;
use threads::shared;
use Thread::Queue;

my $defaultUseThreads = `sysctl -n hw.ncpu`;
$defaultUseThreads = 0 if $defaultUseThreads == 1;
my $useThreads = $defaultUseThreads;

 my $cityCount = 0;

my @allCities = ();

my $thresholdCount = 30;

sub readRawFile {
    my $rawFile = shift;
    open RAW, $rawFile
      or die "Couldn't read $rawFile\n";
    while (<RAW>) {
	chomp;   # Remove trailing NL
	die "Delimiter '+' appears in data line:\n" . $_ if /\+/;
	my ($geonameid, $name, $asciiname, $alternatenames, $latitude, $longitude, $featureClass, $featureCode,
	    $countryCode, $cc2, $admin1Code, $admin2Code, $admin3Code, $admin4Code, $population, $elevation, $gtopo30, $timezoneID, $modDate) = split /\t/;
        push @allCities, [$name, $population];
    }
}

my @overflowCities :shared = ();
my $overflowCities :shared = \@overflowCities;

sub checkCity {
    my $city = shift;
    my $cityIndex = shift;
    my $cityName = $$city[0];
    #warn "Checking '$cityName'\n";
    my @matchingCities = ();
    my $matchingIndex = 0;
    foreach my $matchCity (@allCities) {
        if ($$matchCity[0] =~ m%^$cityName%) {
            push @matchingCities, [$matchCity, $matchingIndex];
        }
        $matchingIndex++
    }
    if (scalar @matchingCities > $thresholdCount) {
        my @sortedList = sort {
            ${$$b[0]}[1] <=> ${$$a[0]}[1]
        } @matchingCities;
        my $sortIndex = 0;
        my $found = 0;
        foreach my $match (@sortedList) {
            if ($$match[1] == $cityIndex) {
                my $matchCount = scalar @sortedList;
                my $population = ${$$match[0]}[1];
                my @overflowCity:shared = ("$cityName($cityIndex)", $sortIndex, $population);
                warn "$sortIndex of $matchCount (pop $population): $cityName\n";
                push @$overflowCities, \@overflowCity;
                die "Multiple instances of cityIndex $cityIndex for '$cityName]' in list??" if $found;
                $found = 1;
            }
            $sortIndex++;
        }
        die "No instances of cityIndex $cityIndex for '$cityName' in list??" if !$found;
    }
}

readRawFile("../data/cities1000.txt");
#readRawFile("/tmp/foo.txt");

my $workQueue;
my @threads;
my $didOne = 0;

if ($useThreads) {
    printf "Using %d thread%s\n", $useThreads, ($useThreads == 1) ? "" : "s";
    $workQueue = new Thread::Queue;
    for (my $i = 0; $i < $useThreads; $i++) {
        push @threads, new threads(sub {
                                       while (my $checkCityDescriptor = $workQueue->dequeue()) {
                                           checkCity $$checkCityDescriptor[0], $$checkCityDescriptor[1];
                                       }
                                   });
    }
}

sub submitCity {
    my $city = shift;
    my $cityIndex = shift;
    $workQueue->enqueue([$city, $cityIndex]);
}

my $cityIndex = 0;
foreach my $checkCity (@allCities) {
    if ($useThreads) {
        submitCity $checkCity, $cityIndex++;
    } else {
        checkCity $checkCity, $cityIndex++;
    }
}


if ($useThreads) {
    for (my $i = 0; $i < $useThreads; $i++) {
	$workQueue->enqueue(undef);
    }
    warn("Waiting...\n");
    for (my $i = 0; $i < $useThreads; $i++) {
	my $thread = shift @threads;
	if ($thread->join()) {
	    $didOne = 1;
	}
    }
}
my $overflowLength = scalar @$overflowCities;
warn("Done waiting $overflowLength\n");

my @sortedOverflows = sort {
    $$b[1] <=> $$a[1]
} @$overflowCities;

my $maxPopSoFar = 0;
foreach my $city (@sortedOverflows) {
    my $pop = $$city[2];
    if ($pop > $maxPopSoFar) {
        $maxPopSoFar = $pop;
    }
    warn sprintf "Rank in results: %3d, population: %8d, max pop to this point: %8d, %s\n", $$city[1], $$city[2], $maxPopSoFar, $$city[0];
}
