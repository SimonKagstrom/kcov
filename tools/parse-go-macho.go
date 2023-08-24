package main

import (
	"debug/dwarf"
	"debug/macho"
	"fmt"
	"io"
	"os"
)

func panicIfErr(err error) {
	if err != nil {
		panic(err)
	}
}

func main() {
	executable := os.Args[1]
	exe, err := macho.Open(executable)
	panicIfErr(err)
	dw, err := exe.DWARF()
	panicIfErr(err)
	reader := dw.Reader()
	for index := 0; ; index++ {
		entry, err := reader.Next()
		panicIfErr(err)
		if entry == nil {
			break
		}
		if entry.Tag != dwarf.TagCompileUnit {
			continue
		}
		lrd, err := dw.LineReader(entry)
		panicIfErr(err)
		for {
			var e dwarf.LineEntry
			err := lrd.Next(&e)
			if err == io.EOF {
				break
			}
			panicIfErr(err)
			fmt.Printf("%s:%d %x\n", e.File.Name, e.Line, e.Address)
		}
	}
}
