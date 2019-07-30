# Author: Joao Gabriel Quaresma
# 2019

import os
import pprint
import xml.etree.ElementTree as ET
import string
import datetime

now = datetime.datetime.now()

# Allow/Not Allow to save jmx test file for option 3.
NOT_SAVE_FILE_JMX_FOR_OPTION_3 = True

# Const options parameters
GENERATE_COMPLETE_REPORT = 1
GENERATE_ONLY_CSV_FILE = 2
ONLY_RESULTS = 3

# Change the parameters
NUM_OF_THREADS = 1
LOOPS = 1
RAMP_TIME = 1
DOMAIN_IP = 'localhost'
PORT = 1026
PATH_SERVICE = '/ngsi-ld/v1/entities/'
HTTP_VERB = 'GET'

OUTPUT_FILE_NAME = HTTP_VERB + '_num_threads_' + \
    str(NUM_OF_THREADS) + '_loops_' + \
    str(LOOPS) + '_ramp_time_' + str(RAMP_TIME) + '_' + str(now.isoformat())

BODY_DATA = """
{
    "brandName": {
        "type": "Property",
        "value": "Chevrolet Spin"
    }
}
"""
# defineParams: Create a structure of folders for HTML Report or just .csv file
# @params : isCreateHTMLReport - Works like a switch for choose the structure


def defineParams(tree):
    root = tree.getroot()

    if HTTP_VERB != "GET" or HTTP_VERB != "POST" or HTTP_VERB != "PATCH" or HTTP_VERB != "DELETE":
        print('ERROR:', 'Use a Orion-LD Broker supported HTTP verb.')
        os.abort()
        

    for item in root.iter('stringProp'):
        if item.attrib['name'] == "ThreadGroup.num_threads":
            item.text = str(NUM_OF_THREADS)
        if item.attrib['name'] == "LoopController.loops":
            item.text = str(LOOPS)
        if item.attrib['name'] == "ThreadGroup.ramp_time":
            item.text = str(RAMP_TIME)
        if item.attrib['name'] == "HTTPSampler.domain":
            item.text = str(DOMAIN_IP)
        if item.attrib['name'] == "HTTPSampler.port":
            item.text = str(PORT)
        if item.attrib['name'] == "HTTPSampler.path":
            item.text = str(PATH_SERVICE)
        if item.attrib['name'] == "HTTPSampler.method":
            item.text = str(HTTP_VERB)

        if HTTP_VERB == "GET":
            if item.attrib['name'] == "Argument.value":
                item.text = ''
        elif HTTP_VERB == "POST":
            if item.attrib['name'] == "Argument.value":
                item.text = BODY_DATA
        elif HTTP_VERB == "PATCH" or HTTP_VERB == "DELETE":
            if item.attrib['name'] == "Header.value":
                item.text = 'application/json'
            if item.attrib['name'] == "Argument.value":
                item.text = BODY_DATA


# createFolders: Create a structure of folders for HTML Report or just .csv file
# @params : isCreateHTMLReport - Works like a switch for choose the structure


def createFolders(isCreateHTMLReport):
    if not os.path.exists('output'):
        os.mkdir('output')
    if isCreateHTMLReport:
        os.mkdir('output/' + OUTPUT_FILE_NAME + '_Report')
        os.mkdir('output/' + OUTPUT_FILE_NAME + '_Report/Report')
    else:
        os.mkdir('output/' + OUTPUT_FILE_NAME)

# executeTest: execute one of the options of menu for JMeter Test


def executeTest(operation, tree):
    if operation == GENERATE_COMPLETE_REPORT:
        createFolders(True)
        tree.write('output/' + OUTPUT_FILE_NAME +
                   '_Report/' + OUTPUT_FILE_NAME + '.jmx')
        os.system('jmeter -n -t output/' + OUTPUT_FILE_NAME + '_Report/' + OUTPUT_FILE_NAME + '.jmx -l output/' + OUTPUT_FILE_NAME +
                  '_Report/' + OUTPUT_FILE_NAME + '.csv -e -o output/' + OUTPUT_FILE_NAME + '_Report/Report')
    elif operation == GENERATE_ONLY_CSV_FILE:
        createFolders(False)
        tree.write('output/' + OUTPUT_FILE_NAME +
                   '/' + OUTPUT_FILE_NAME + '.jmx')
        os.system('jmeter -n -t output/' + OUTPUT_FILE_NAME + '/' +
                  OUTPUT_FILE_NAME + '.jmx -l output/' + OUTPUT_FILE_NAME + '/' + OUTPUT_FILE_NAME + '.csv')
    elif operation == ONLY_RESULTS:
        tree.write(OUTPUT_FILE_NAME + '.jmx')
        os.system('jmeter -n -t ' + OUTPUT_FILE_NAME + '.jmx')
        if NOT_SAVE_FILE_JMX_FOR_OPTION_3:
            os.remove(OUTPUT_FILE_NAME+'.jmx')

# execute: It is a selector to execute one of the options of menu for JMeter Test


def execute(op, tree):
    if op.isdigit():
        op = int(op)
        if op == GENERATE_COMPLETE_REPORT:
            executeTest(GENERATE_COMPLETE_REPORT, tree)
        elif op == GENERATE_ONLY_CSV_FILE:
            executeTest(GENERATE_ONLY_CSV_FILE, tree)
        elif op == ONLY_RESULTS:
            executeTest(ONLY_RESULTS, tree)
        elif op == 0:
            exit
        else:
            print("ERROR:", "Type any of the available options")
    else:
        print("ERROR:", "Type any of the available options")


def main():
    tree = ET.parse('testTemplate.jmx')
    defineParams(tree)

    print('1 - Generate complete report (.csv + HTML)')
    print('2 - Generate only .csv file')
    print('3 - Only results on terminal')
    print('0 - Exit')
    op = input('Chose the option: ')

    execute(op, tree)


if __name__ == "__main__":
    main()
