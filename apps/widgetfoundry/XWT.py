import PicoGUI, string, WidgetProperties

# Settings for outputting XWTs
indentSize = 4
maxInlineLength = 70


def getIndent(indentLevel):
    return ' ' * (indentSize * indentLevel)


# Returns the attribute list, with separator before each pair
def getAttributes(properties, separator=' '):
    attr = ''
    for prop in properties:
        attr += '%s%s="%s"' % (separator, prop, properties[prop])
    return attr


def getChildren(widget, app):
    children = []
    # This try block ignores children not owned by this app,
    # such as embedded applications.
    h = widget.server.traversewidget(widget.handle,'children',0)
    try:
        while h:
            children.append(app.getWidget(h))
            h = widget.server.traversewidget(h,'forward',1)
    except (KeyError, PicoGUI.responses.ProtocolError):
        pass
    return children


def dumpWidget(widget, app, indentLevel=0):
    indent = getIndent(indentLevel)
    properties = WidgetProperties.getList(widget, app).copy()
    children = getChildren(widget,app)

    # Text goes in the tag's data unless there are children
    if len(children) == 0 and properties.has_key('text'):
        data = properties['text']
        del properties['text']
    else:
        data = ''

    xwt = "%s<%s%s" % (indent, widget.type, getAttributes(properties))

    # Was that too long? We can put properties on individual lines
    if len(xwt) > maxInlineLength:
        separator = '\n' + getIndent(indentLevel+1)
        xwt = '%s<%s%s%s' % (indent, widget.type, getAttributes(properties,separator), separator)

    # If we have no widgets or data, make this a self-closing tag and exit
    if len(children) == 0 and len(data) == 0:
        return xwt + '/>\n'
    else:
        xwt += '>\n'

    if len(data) > 0:
        xwt += getIndent(indentLevel+1) + data + '\n'

    for child in children:
        xwt += dumpWidget(child, app, indentLevel+1)

    return '%s%s</%s>\n' % (xwt, indent, widget.type)
