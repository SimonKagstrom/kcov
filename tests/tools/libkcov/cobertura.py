import xml.dom.minidom


def readFile(name):
    f = open(name, "r")
    data = f.read()
    f.close()

    return data


def lookupClassName(dom, name):
    tags = dom.getElementsByTagName("class")

    for tag in tags:
        nameAttr = tag.attributes["name"]
        if nameAttr.value.startswith(name):
            return tag

    return None


def lookupHitsByLine(classTag, lineNr):
    tags = classTag.getElementsByTagName("line")

    for tag in tags:
        numberAttr = tag.attributes["number"]
        if int(numberAttr.value) == int(lineNr):
            hitsAttr = tag.attributes["hits"]
            return int(hitsAttr.value)

    return None


def parse(data):
    # parse the xml you got from the file
    dom = xml.dom.minidom.parseString(data)

    return dom


def parseFile(filename):
    # parse the xml you got from the file
    dom = xml.dom.minidom.parseString(readFile(filename))

    return dom


def hitsPerLine(dom, fileName, lineNr):
    fileName = fileName.replace(".", "_").replace("-", "_")
    fileTag = lookupClassName(dom, fileName)

    if fileTag is not None:
        hits = lookupHitsByLine(fileTag, lineNr)
        if hits is not None:
            return hits

    return None
