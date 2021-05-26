package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"rogchap.com/v8go"
	"strings"
)

type CompiledTS struct {
	Outputtext    string        `json:"outputText"`
	Diagnostics   []interface{} `json:"diagnostics"`
	Sourcemaptext string        `json:"sourceMapText"`
}

func main() {
	ts, _ := ioutil.ReadFile("node_modules/typescript/lib/typescript.js")

	ctx, _ := v8go.NewContext()                                            // creates a new V8 context with a new Isolate aka VM
	ctx.RunScript(string(ts), "node_modules/typescript/lib/typescript.js") // executes a script on the global context
	ctx.RunScript("let source = \"\";", "compile.js")                      // executes a script on the global context

	//fmt.Printf("addition result: %v", tsData)

	http.HandleFunc("/src/", tsServer(ctx))
	http.HandleFunc("/", indexHTML)
	http.ListenAndServe(":7000", nil)

}

func indexHTML(writer http.ResponseWriter, request *http.Request) {
	filename := getFilename(request)
	prefix := getExt(filename)
	index, err := ioutil.ReadFile(filename)
	if err != nil {
		fmt.Fprint(writer, err.Error())
	}
	SetContentType(writer, prefix)
	fmt.Fprint(writer, string(index))
}

func SetContentType(writer http.ResponseWriter, prefix string) {
	writer.Header().Add("Content-Type", map[string]string{
		"js":   "application/javascript",
		"html": "text/html",
	}[prefix])
}

func getExt(filename string) string {
	parts := strings.Split(filename, ".")
	return parts[len(parts)-1]
}

func getFilename(request *http.Request) string {
	filename := "./public" + request.RequestURI
	if request.RequestURI == "/" {
		filename = "./public/index.html"
	}
	return filename
}

func tsServer(ctx *v8go.Context) func(http.ResponseWriter, *http.Request) {
	return func(writer http.ResponseWriter, request *http.Request) {

		fmt.Println("Compile: ", request.RequestURI)

		source, _ := ioutil.ReadFile("./" + strings.TrimSuffix(request.RequestURI, ".map") + ".ts")
		sourceJSon, _ := json.Marshal(string(source))

		log.Println("source = " + string(sourceJSon) + ";")

		_, err := ctx.RunScript("source = "+string(sourceJSon)+";", "compile.js") // any functions previously added to the context can be called
		if err != nil {
			log.Println("source error", err)
			return
		}
		val, err := ctx.RunScript("JSON.stringify(ts.transpileModule(source, { compilerOptions: { module: ts.ModuleKind.CommonJS, sourceMap: true }}));", "compile.js") // any functions previously added to the context can be called
		if err != nil {
			log.Println("compile error", err)
			return
		}

		tsData := CompiledTS{}
		json.Unmarshal([]byte(val.String()), &tsData)

		if strings.HasSuffix(request.RequestURI, ".map") {
			writer.Header().Add("content-type", "application/json")
			fmt.Fprint(writer, tsData.Sourcemaptext)
			return
		}
		writer.Header().Add("content-type", "application/javascript")
		js := strings.ReplaceAll(tsData.Outputtext, "require(", "await require(")
		js = strings.Replace(js, "module.js", strings.TrimPrefix(request.RequestURI, "/src/"), 1)
		fmt.Fprint(writer, js)
		return
	}
}
