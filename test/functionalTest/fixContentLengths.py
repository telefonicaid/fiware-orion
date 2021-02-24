#!/usr/bin/python
# -*- coding: latin-1 -*-
# Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
#
# This file is part of Orion Context Broker.
#
# Orion Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# iot_support at tid dot es


from getopt import getopt, GetoptError

from operator import add
from re import match
from os import rename, remove, listdir
from os.path import splitext, isdir, basename, dirname, isfile
from sys import argv
from tempfile import NamedTemporaryFile

__author__ = 'fermin'


#####################
# Functions program #
#####################

def msg(m):
    """
    Print message if verbose mode is enabled

    :param m: message to print
    """
    global verbose

    if verbose:
        print(m)


def usage_and_exit(m):
    """
    Print usage message and exit

    :param m: optional error message to print
    """

    if m != '':
        print(m)
        print()

    usage()
    exit(1)


def usage():
    """
    Print usage message
    """

    print('Modifies each "Content-Length" line in .test files, based on the same values in the corresponding .out file')
    print('')
    print('Usage: %s -f <file> [-c] [-d] [-v] [-u]' % basename(__file__))
    print('')
    print('Parameters:')
    print('  -f <file>: .test or .out file to modify. Alternatively, it can be a directory in which case')
    print('             all the .test/.out files in that directory are checked, recursively.')
    print('  -d: dry-run mode, i.e. .test files are not modified')
    print('  -c: only-check mode, i.e. just resport if the file differs only in Content-Length or not')
    print('  -v: verbose mode')
    print('  -u: print this usage message')


def patch_content_lengths(file_name, cl):
    """
    Patch the Content-Length lines in file_name with the data comming from the cl argument,
    assuming it is a list of pairs as returned by content_length_extract(), i.e. using the
    second argument in each pair.

    The patch is based in the technique of writing a temporal file, then replace the original
    one with it.

    It is assumed that cl is ok before invoking this function, i.e. check_same_lines() has been
    previously named.

    :param file_name: the .test file to patch
    :param cl: the list of pair to be used for patching
    """

    msg('  - patching file %s' % file_name)

    n = 0

    try:
        file_temp = NamedTemporaryFile(delete=False)
        for line in open(file_name):
            m = match(r'^Content-Length: (\S+)', line)
            if m is not None:
                if cl[n][1] == '*':
                    # This case corresponds to 'Content-Lenght: REGEX(\d+)', so it must not be touched
                    file_temp.write(line)
                    msg("    -skipping patch as '*' was found")
                else:
                    file_temp.write('Content-Length: %d\n' % cl[n][1])
                    msg('    - patching "Content-Length: %d"' % cl[n][1])
                n += 1
            else:
                file_temp.write(line)

        file_temp.close()

        # Remove old file, replacing by the edited one
        remove(file_name)
        rename(file_temp.name, file_name)
    except Exception as err:
        print('* error processing file %s: %s' % (file_name, str(err)))


def content_length_extract(file_name):
    """
    Open file passed as argument, looking for Content-Lengh lines. A list of pairs is removed, the
    first item in each pair being a line number and the second item the Content-Lenght value

    :param file_name: file to be processed
    :return: a list of pairs as described above
    """

    r = []
    number_lines = 0

    try:
        for line in open(file_name):
            number_lines += 1
            m = match(r'^Content-Length: (\S+)', line)
            if m is not None:
                try:
                    cl = int(m.group(1))
                    msg('    - push [%d, %d]' % (number_lines, cl))
                    r.append([number_lines, cl])
                except ValueError:
                    # Some times we have 'Content-Length: REGEX(\d+)'. It that case, we put a '*' in the pair
                    if m.group(1).startswith('REGEX('):
                        msg('    - push [%d, *]' % number_lines)
                        r.append([number_lines, '*'])
                    else:
                        raise 'invalid Content-Length value: <%s> ' % m.group(1)

    except Exception as err:
        print('* error processing file %s: %s' % (file_name, str(err)))

    return r


def check_same_lines(r1, r2):
    """
    Assuming arguments are list of pairs as generated by content_length_extract(), check that
    the have the same number of items and that the lines (firs item in each pair) match.

    :param r1: first list of pairs to compare
    :param r2: second list of pairs to compare
    :return: a list with the following items:
       * True if list check is ok, False otherwise
       * number of not equal content-length lines (i.e. number of lines that need a change) (only makes sense
         if first item is True)
       * accumulated lengths in r2 (only makes sense if first item is True)
       * accumulated lengths in r2 (only makes sense if first item is True)
    """

    not_equal = 0
    acc_r1 = 0
    acc_r2 = 0

    if len(r1) != len(r2):
        msg('  - number of content-length entries does no match, skipping!')
        return [False, 0, 0, 0]

    for i in range(0, len(r1)):
        if r1[i][0] != r2[i][0]:
            msg('  - item #%d does not match: .test line %d vs .out line %d, skipping!' % (i, r1[i][0], r2[i][0]))
            return [False, 0, 0, 0]
        else:
            r1_val = r1[i][1]
            r2_val = r2[i][1]
            if r1_val == '*' or r2_val == '*':
                msg("  - item #%d uses '*', so no actual check done" % i)
            elif r1_val != r2_val:
                not_equal += 1
                acc_r1 += r1_val
                acc_r2 += r2_val

    return [True, not_equal, acc_r1, acc_r2]


def process_dir(dir_name, file_mode):
    """
    Recursive dir processing.

    :param dir_name: directory to be processed
    :param file_mode: working mode (either PATCH, DRY_RUN or ONLY_CHECK)
    :return: a list with the following items:
       * True if the process was ok, False otherwise
       * number of processed files (only makes sense if first item is True)
       * number of not equal content-length lines (i.e. number of lines that were changed) (only makes sense
         if first item is True)
       * accumulated lengths in .regexpect (only makes sense if first item is True)
       * accumulated lengths in .out (only makes sense if first item is True)
    """

    # To be sure directory hasn't a trailing slash
    dir_name_clean = dir_name.rstrip('/')

    accum = [0, 0, 0, 0, 0]

    try:
        list_files = listdir(dir_name_clean)
    except Exception as err:
        print('* error processing directory %s: %s' % (dir_name_clean, str(err)))
        return accum

    for file_name in list_files:

        file_name = dir_name_clean + '/' + file_name

        if isdir(file_name):
            accum = map(add, accum, process_dir(file_name, file_mode))
        else:
            extension = splitext(basename(file_name))[1]
            if extension == '.out':
                accum = map(add, accum, process_file(file_name, file_mode))

    return accum


def process_file(file_name, file_mode):
    """
    Process the file pass as argument, In can be either .test or .out. The process is as follows

    1. Get the file basename (i.e. without .out or .test)
    2. Check that basename + .test exist, otherwise return
    3. Check that basename + .out exist, otherwise return
    4. Get all the [line, Content-Length value] pairs in the .test file
    5. Get all the [line, Content-Length value] pairs in the .out file
    6. Check that lines in both cases match
    7. Go through .test modifying Content-Length based the results in the .out file

    :param file_name: file to be processed
    :param file_mode: working mode (either PATCH, DRY_RUN or ONLY_CHECK)
    :return: a list with the following items:
       * True if the process was ok, False otherwise
       * number of processed files (always 1) (only makes sense if first item is True)
       * number of not equal content-length lines (i.e. number of lines that were changed) (only makes sense
         if first item is True)
       * accumulated lengths in .regexpect (only makes sense if first item is True)
       * accumulated lengths in .out (only makes sense if first item is True)
    """

    msg('* processing file %s' % file_name)

    # Step 1 to 3
    path = dirname(file_name)
    basename_file = splitext(basename(file_name))[0]

    file_test = '%s/%s.test' % (path, basename_file)
    file_out = '%s/%s.out' % (path, basename_file)
    file_exp = '%s/%s.regexpect' % (path, basename_file)

    for f in [file_test, file_out, file_exp]:
        if not isfile(f):
            msg('  - cannot find %s, skipping!' % f)
            return [False, 0, 0, 0, 0]

    # Step 4 and 5
    msg('  - extracting content length in .regexpect file')
    cl_exp = content_length_extract(file_exp)
    msg('  - extracting content length in .out file')
    cl_out = content_length_extract(file_out)

    # Step 6
    [result_check, not_equal, acc_exp, acc_out] = check_same_lines(cl_exp, cl_out)
    if mode == 'ONLY_CHECK':
        if result_check:
            print('= OK:  %s' % file_out)
        else:
            print('= NOK: %s' % file_out)
        return [False, 0, 0, 0, 0]
    else:
        if not result_check:
            return [False, 0, 0, 0, 0]

    # Step 7
    if file_mode == 'DRY_RUN':
        msg('  - dry run mode: not touching file')
    else:
        patch_content_lengths(file_test, cl_out)

    msg('  - changes: %d, length .regexpect: %d, length .out %d' % (not_equal, acc_exp, acc_out))

    return [True, 1, not_equal, acc_exp, acc_out]


################
# Main program #
################


try:
    opts, args = getopt(argv[1:], 'f:cdvu', [])

    # Defaults
    file = ''
    verbose = False
    mode = 'PATCH'

    for opt, arg in opts:
        if opt == '-u':
            usage()
            exit(0)
        elif opt == '-v':
            verbose = True
        elif opt == '-d':
            mode = 'DRY_RUN'
        elif opt == '-c':
            mode = 'ONLY_CHECK'
        elif opt == '-f':
            file = arg
        else:
            usage_and_exit('')

    if file == '':
        usage_and_exit('missing -f parameter')

    if isdir(file):
        result = process_dir(file, mode)
    else:
        result = process_file(file, mode)

    files = result[1]
    changes = result[2]
    result_exp = result[3]
    result_out = result[4]

    msg('TOTAL: files: %d, changes: %d, length .regexpect: %d, length .out %d'
        % (files, changes, result_exp, result_out))
except GetoptError:
    usage_and_exit('wrong parameter')
