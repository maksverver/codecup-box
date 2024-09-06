
# Returns a generator that removes comments from lines (starting with '#'),
# strips trailing whitespace, and yields only the nonempty resulting lines.
def StripComments(file):
    for line in file:
        i = line.find('#')
        if i >= 0: line = line[:i]
        line = line.rstrip()
        if line != '': yield line
