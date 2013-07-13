utils
=====

This is a collection of utilities I found useful.

pyl
===

`pyl [-vs] [--use filenames] input_file [block1] [block2] [block3]`

pyl (Python Line Processor) applies a python expression to each line of input file. 
If input_file is an html file, pyl will find a table in the file, and apply
the python expression to rows of the table.

If no blocks are specified, that python expression is read from stdin.
If one block is specified, that blocks is evaulated for each line of the file.
If two blocks are provided, the first block is executed for each line, and the second
is executed after the entire file is processed.
If three blocks are provided, the first block is executed before the file is processed,
the second for each line, and the third after the entire file is ran.
This is useful for maintaining sums, etc.

The command line options are:
* -v prints all lines that pass the filter (explained below).
* -s don't print the number of lines that pass the filter.
* --use filenames: If you want to read from another file at the same time (to compare the files, for example)
--use will preload those files so you don't read from the file every loop.

The python syntax used in the blocks are normal syntax. `;;` can be used to separate lines.
All pyl variables begin with an underscore, so in your code, you can use whatever variable names you want
that doesn't begin with an underscore.
pyl has some special functions or variables that your code can access:
* `_line`: the current line, stripped of all spaces. For tables in html files, this is given by `_row`,
a list of cells in that row.
* `_n`: the current line number.
* `_filter`: a Boolean you can set to indicate if the line satisfies some criterion. In the end,
the number of lines that matches will get outputted. `_filter` is added to address a common use-case,
but you don't need to use `_filter` or even care about it.
* `_count`: the number of lines that have passed the filter so far.
* `column(n,sep=None)`: splits `_line` according to separator (default to all whitespace), and returns
n-th entry. Related to `columns(sep=None)`, which returns a list of columns.
* `_read(filename)`: Reads file. If `--use` is set, then `_read` will load from a cached version of the file.
* `_in(filename,text)`: returns true if text is one of the lines of file.

Examples:

* `pyl test.txt ""` prints number of lines in test.txt.
* `pyl -vs test.txt "_filter=_n<=10"` prints the first 10 lines of the file. This is the same as `head test.txt`.
* `pyl test.txt "_filter=int(column(1))>18"` will print the number of lines in test.txt with the first column
greater than 18 (where columns are separated by spaces; to use comma-separated columns, use `column(1,',')`)
Columns accept negative indices as well, in the obvious Python way. `_filter` is
* `pyl -vs test.t "_filter=int(column(1))>18" will print the lines in test.txt that satisfy the condition.
This is just a shortcut for `pyl test.txt "_filter=int(column(1))>18;;if _filter: print _line"`.
* `pyl test.html "_filter=int(_row[0])>18"` will look for a table in test.html and parse that table, looping
through the rows of that table and applying the provided block to each row.
