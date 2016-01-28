#!/usr/bin/env python

import sys
import xml.dom.minidom

#all these imports are standard on most modern python implementations

def readFile(name):
    f = open(name,'r')
    data = f.read()
    f.close()

    return data


def lookupClassName(dom, name):
    tags = dom.getElementsByTagName('class')

    for tag in tags:
        nameAttr = tag.attributes["name"]
        if nameAttr.value == name:
            return tag

    return None

def lookupHitsByLine(classTag, lineNr):
    tags = classTag.getElementsByTagName('line')

    for tag in tags:
        numberAttr = tag.attributes["number"]
        if int(numberAttr.value) == int(lineNr):
            hitsAttr = tag.attributes["hits"]
            return int(hitsAttr.value)

    return None


def parse(data):
    #parse the xml you got from the file
    dom = xml.dom.minidom.parseString(data)

    return dom

def parseFile(filename):
    #parse the xml you got from the file
    dom = xml.dom.minidom.parseString(readFile(filename))

    return dom

def hitsPerLine(dom, fileName, lineNr):
    fileName = fileName.replace(".", "_").replace("-", "_")

    fileTag = lookupClassName(dom, fileName)

    if fileTag != None:
        hits = lookupHitsByLine(fileTag, lineNr)
        if hits != None:
            return hits

    return None

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print "Usage: lookup-class-line <in-file> <filename> <lineNr>"
        sys.exit(1)


    fileName = sys.argv[2]
    line = int(sys.argv[3])

    data = readFile(sys.argv[1])

    dom = parse(data)
    fileTag = lookupClassName(dom, fileName)

    if fileTag != None:
        hits = lookupHitsByLine(fileTag, line)
        if hits != None:
            print hits
            sys.exit(0)

    print "nocode"
