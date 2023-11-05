#!/usr/bin/perl -w

use strict;

use File::Basename;
use File::Copy "cp";

my $inputDir = (fileparse $0)[1];  # The directory containing this script
$inputDir =~ s%/$%%o;
my $outputDir = "../assets";

print "Updating assets directory from $inputDir...\n";

sub filesAreIdentical {
    my ($file1, $file2) = @_;
    return system("cmp -s \"$file1\" \"$file2\"") == 0;
}

sub checkFile {
    my $file = shift;
    my $inputFile = "$inputDir/$file";
    my $outputFile = "$outputDir/$file";
    $outputFile .= ".png";
    if (filesAreIdentical($inputFile, $outputFile)) {
        return;
    }
    my $message;
    if (-e $outputFile) {
        unlink $outputFile;
        $message = "Updated $outputFile\n";
    } else {
        $message = "Created new $outputFile\n";
    }
    cp $inputFile, $outputFile
      or die "Couldn't copy $inputFile to $outputFile: $!\n";
    warn $message;
}

opendir DIR, $inputDir
  or die "Couldn't read directory $inputDir: $!\n";
my @entries = grep /\.dat$|\.sum$/, readdir DIR;
closedir DIR;

foreach my $entry (@entries) {
    checkFile $entry;
}
