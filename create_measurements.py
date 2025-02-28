#!/usr/bin/env python

# Modified from https://github.com/gunnarmorling/1brc/blob/db064194be375edc02d6dbcd21268ad40f7e2869/src/main/python/create_measurements.py

#
#  Copyright 2023 The original authors
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

# Based on https://github.com/gunnarmorling/1brc/blob/main/src/main/java/dev/morling/onebrc/CreateMeasurements.java

import os
import sys
import random
import time
from dataclasses import dataclass
from argparse import ArgumentParser
from typing import List

@dataclass
class Config:
    num_rows: int
    file_path: str
    station_names_file_path: str

def parse_args() -> Config:
    parser = ArgumentParser(description="Create measurements file to be used in 1brc.")
    parser.add_argument("-f", "--file-path", type=str, default="data/measurements.txt")
    parser.add_argument("-n", "--num-rows", type=int, default=10_000)
    parser.add_argument("-s", "--station-names", type=str, default="data/weather_stations.csv")

    args = parser.parse_args()
    return Config(num_rows=args.num_rows, file_path=args.file_path, station_names_file_path=args.station_names)

def build_weather_station_name_list(file_path: str):
    """
    Grabs the weather station names from example data provided in repo and dedups
    """
    station_names = []
    with open(file_path, 'r', encoding='utf-8') as file:
        file_contents = file.read()
    for station in file_contents.splitlines():
        if "#" not in station:
            station_names.append(station.split(';')[0])
    return list(set(station_names))


def convert_bytes(num):
    """
    Convert bytes to a human-readable format (e.g., KiB, MiB, GiB)
    """
    for x in ['bytes', 'KiB', 'MiB', 'GiB']:
        if num < 1024.0:
            return "%3.1f %s" % (num, x)
        num /= 1024.0


def format_elapsed_time(seconds):
    """
    Format elapsed time in a human-readable format
    """
    if seconds < 60:
        return f"{seconds:.3f} seconds"
    elif seconds < 3600:
        minutes, seconds = divmod(seconds, 60)
        return f"{int(minutes)} minutes {int(seconds)} seconds"
    else:
        hours, remainder = divmod(seconds, 3600)
        minutes, seconds = divmod(remainder, 60)
        if minutes == 0:
            return f"{int(hours)} hours {int(seconds)} seconds"
        else:
            return f"{int(hours)} hours {int(minutes)} minutes {int(seconds)} seconds"


def estimate_file_size(weather_station_names, num_rows_to_create):
    """
    Tries to estimate how large a file the test data will be
    """
    total_name_bytes = sum(len(s.encode("utf-8")) for s in weather_station_names)
    avg_name_bytes = total_name_bytes / float(len(weather_station_names))

    # avg_temp_bytes = sum(len(str(n / 10.0)) for n in range(-999, 1000)) / 1999
    avg_temp_bytes = 4.400200100050025

    # add 2 for separator and newline
    avg_line_length = avg_name_bytes + avg_temp_bytes + 2

    human_file_size = convert_bytes(num_rows_to_create * avg_line_length)

    return f"Estimated max file size is:  {human_file_size}."


def build_test_data(file_path: str, weather_station_names: List[str], num_rows_to_create: int):
    """
    Generates and writes to file the requested length of test data
    """
    start_time = time.time()
    coldest_temp = -99.9
    hottest_temp = 99.9
    station_names_10k_max = random.choices(weather_station_names, k=10_000)
    batch_size = 10000 # instead of writing line by line to file, process a batch of stations and put it to disk
    chunks = num_rows_to_create // batch_size
    print('Building test data...')

    try:
        with open(file_path, 'w', encoding='utf-8') as file:
            progress = 0
            for chunk in range(chunks):
                
                batch = random.choices(station_names_10k_max, k=batch_size)
                prepped_deviated_batch = '\n'.join([f"{station};{random.uniform(coldest_temp, hottest_temp):.1f}" for station in batch]) # :.1f should quicker than round on a large scale, because round utilizes mathematical operation
                file.write(prepped_deviated_batch + '\n')
                
                # Update progress bar every 1%
                if (chunk + 1) * 100 // chunks != progress:
                    progress = (chunk + 1) * 100 // chunks
                    bars = '=' * (progress // 2)
                    sys.stdout.write(f"\r[{bars:<50}] {progress}%")
                    sys.stdout.flush()
        sys.stdout.write('\n')
    except Exception as e:
        print("Something went wrong. Printing error info and exiting...")
        print(e)
        exit()
    
    end_time = time.time()
    elapsed_time = end_time - start_time
    file_size = os.path.getsize(file_path)
    human_file_size = convert_bytes(file_size)
 
    print(f"Test data successfully written to {file_path}")
    print(f"Actual file size:  {human_file_size}")
    print(f"Elapsed time: {format_elapsed_time(elapsed_time)}")


def main():
    config = parse_args()
    weather_station_names = build_weather_station_name_list(config.station_names_file_path)
    print(estimate_file_size(weather_station_names, config.num_rows))
    build_test_data(config.file_path, weather_station_names, config.num_rows)
    print("Test data build complete.")


if __name__ == "__main__":
    main()
exit()
