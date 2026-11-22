// Package cfgy evaluates ConFiGifY files using libcfgy.
package cfgy

/*
#cgo CFLAGS: -I${SRCDIR}/../c
#cgo LDFLAGS: -lcfgy
#include "cfgy.h"
#include <stdlib.h>
*/
import "C"
import (
	"errors"
	"os"
	"strings"
	"unsafe"
)

func text(s C.cfgy_string) string {
	return string(unsafe.Slice((*byte)(unsafe.Pointer(s.data)), int(s.size)))
}
func convert(n *C.cfgy_value) any {
	switch n.kind {
	case C.CFGY_NULL:
		return nil
	case C.CFGY_BOOL:
		return n.integer != 0
	case C.CFGY_INT:
		return int64(n.integer)
	case C.CFGY_FLOAT:
		return float64(n.number)
	case C.CFGY_STRING:
		return text(n.string)
	case C.CFGY_LIST:
		items := make([]any, int(n.count))
		children := unsafe.Slice(n.children, len(items))
		for i := range items {
			items[i] = convert(&children[i])
		}
		return items
	case C.CFGY_MAP:
		result := make(map[string]any, int(n.count))
		children := unsafe.Slice(n.children, int(n.count))
		for i := range children {
			result[text(children[i].key)] = convert(&children[i])
		}
		return result
	default:
		panic("unknown cfgy value type")
	}
}

func parse(data []byte, source string) (map[string]any, error) {
	if strings.IndexByte(source, 0) >= 0 {
		return nil, errors.New("NUL in source name")
	}
	name := C.CString(source)
	defer C.free(unsafe.Pointer(name))
	var input *C.char
	if len(data) > 0 {
		input = (*C.char)(unsafe.Pointer(&data[0]))
	}
	doc := C.cfgy_parse(input, C.size_t(len(data)), name)
	defer C.cfgy_free(doc)
	if err := C.cfgy_error(doc); err != nil {
		return nil, errors.New(C.GoString(err))
	}
	return convert(C.cfgy_root(doc)).(map[string]any), nil
}

func Loads(input string) (map[string]any, error) { return parse([]byte(input), "<string>") }

func Load(path string) (map[string]any, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}
	return parse(data, path)
}
