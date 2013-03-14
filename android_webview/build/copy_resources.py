#!/usr/bin/env python
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to copy generated resources to the Android WebView Package.
"""

import argparse
import sys
import shutil
import os

def main(argv):
  argument_parser = argparse.ArgumentParser()
  argument_parser.add_argument('--additional-r-files', dest='additional_r_files', nargs="+")
  argument_parser.add_argument('--additional-r-text-files', dest='additional_r_text_files', nargs="+")
  argument_parser.add_argument('--generated-r-dirs', dest='generated_r_dirs', nargs="+")
  argument_parser.add_argument('--additional-res-packages', dest='additional_res_packages', nargs="+")
  argument_parser.add_argument('--additional-res-dirs', dest='additional_res_dirs', nargs="+")
  argument_parser.add_argument('--output-ant-file', dest='output_ant_file')
  argument_parser.add_argument('--output-dir', dest='output_dir')
  options = argument_parser.parse_args()

  ant_res_dir = os.path.join("${chrome.dir}", "res")

  if os.path.isdir(options.output_dir):
    shutil.rmtree(options.output_dir)
  os.makedirs(options.output_dir)

  r_text_files = []
  for (i, package) in enumerate(options.additional_res_packages):
    output_dir = os.path.join(options.output_dir, package)
    shutil.copytree(options.generated_r_dirs[i], output_dir)
    r_text_files.append(os.path.join(ant_res_dir, package, "R.txt"))
  
  additional_res_dirs = [];
  for (i, dir) in enumerate(options.additional_res_dirs):
    name = "res" + str(i)
    shutil.copytree(dir, os.path.join(options.output_dir, name))
    additional_res_dirs.append(os.path.join(ant_res_dir, name))

  build_file = open(options.output_ant_file, 'w')
  build_file.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
  build_file.write("<project default=\"error\">\n")
  build_file.write("  <path id=\"project.library.res.folder.path\">\n")
  build_file.write("    <filelist files=\"" + " ".join(additional_res_dirs) + "\">\n")
  build_file.write("  </path>\n")
  build_file.write("  <path id=\"project.library.bin.r.file.path\">\n")
  build_file.write("    <filelist files=\"" + " ".join(r_text_files) + "\">\n")
  build_file.write("  </path>\n")
  build_file.write("  <property name=\"project.library.packages\" value=\"" + ";".join(options.additional_res_packages) + "\" />\n")
  build_file.write("</project>\n")
  build_file.close()

if __name__ == '__main__':
  main(sys.argv)
