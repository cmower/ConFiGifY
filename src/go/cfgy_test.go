package cfgy

import (
	"reflect"
	"strings"
	"testing"
)

func TestSharedFixture(t *testing.T) {
	cfg, err := Load("../../tests/conformance.cfgy")
	if err != nil {
		t.Fatal(err)
	}
	mixed := []any{nil, true, false, int64(0), 2.5, "six", []any{}}
	if !reflect.DeepEqual(cfg["mixed"], mixed) || cfg["wide"] != int64(9007199254740993) ||
		cfg["label"] != "café 雪 😀" || cfg["group"].(map[string]any)["a.b"] != int64(7) ||
		!reflect.DeepEqual(cfg["absolute"], []any{int64(3), 2.5, int64(0)}) {
		t.Fatal(cfg)
	}
	if _, err = Loads("x: sqrt(-1)"); err == nil || !strings.Contains(err.Error(), "<string>:1:4:") {
		t.Fatalf("unexpected error: %v", err)
	}
}
