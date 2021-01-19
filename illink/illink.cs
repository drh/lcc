// Copyright Microsoft Corporation. All rights reserved.
// $Id: illink.cs#19 2002/12/11 11:15:24 REDMOND\\drh $

using System;
using System.IO;
using System.Collections;
using System.Diagnostics;

class illink {
	static string[] argv = Environment.GetCommandLineArgs();
	static public void Warning(string format, params object[] args) {
		Console.Error.Write("{0}: ", argv[0]);
		Console.Error.WriteLine(format, args);
	}
	static public void Error(string format, params object[] args) {
		Warning(format, args);
		Environment.Exit(1);
	}
	static void usage() {
		Console.Error.WriteLine("usage: {0} {1}", argv[0],
			"[ -help | -? | -log[=file] | -v | -debug | -o file | -a name | -u | -l file | file ]... ]");
		Environment.Exit(1);
	}
	static illink() {
		Symbols.Ref("exit", "void 'exit'(int32)");
		Symbols.Ref("_setcallerp", "void '_setcallerp'(void*)");
	}
	static int Main(string[] args) {
		string aname = "a.exe", oname = String.Empty;
		int i, nf = 0;
		bool verbose = false;
		TextWriter outfile = Console.Out;
		ArrayList thunks = new ArrayList(), unmanaged = new ArrayList(),
			asmargs = new ArrayList(new string[] { "-nologo", "-quiet" });
		for (i = 0; i < args.Length; i++) {
			switch (args[i]) {
			case "-help": case "-?":
				usage();
				continue;
			case "-log":			// -log
				Log.New(Console.Error, argv);
				continue;
			case "-a":				// -a name
				if (++i >= args.Length)
					usage();
				aname = args[i];
				continue;
			case "-debug":			// -debug
				asmargs.Add(args[i]);
				continue;
			case "-o":				// -o file
				if (++i >= args.Length)
					usage();
				oname = args[i];
				continue;
			case "-u":				// -u
				ListUndefined();
				continue;
			case "-v":				// -v
				verbose = true;
				continue;
			case "-l":				// -l file
				if (++i >= args.Length)
					usage();
				Library.Search(args[i], verbose, Console.Out);
				continue;
			case "-":
				Pass1("-", Console.In, unmanaged);
				nf++;
				continue;
			}
			if (args[i].StartsWith("-log="))
				Log.New(args[i].Substring(5), argv);
			else if (!args[i].StartsWith("-")) {
				try {
					StreamReader input = new StreamReader(args[i]);
					Pass1(args[i], input, unmanaged);
					input.Close();
					asmargs.Add(args[i]);
				} catch {
					Warning("can't read {0}", args[i]);
				}
				nf++;
			}
		}
		if (nf == 0)
			Pass1("-", Console.In, unmanaged);
		if (Symbols.refs.Count > 0) {
			Warning("undefined symbols:");
			ListUndefined();
		}
		if (oname == String.Empty)
			oname = Path.GetTempFileName();
		if (oname == "-")
			Pass2(aname, unmanaged, Console.Out);
		else {
			try {
				TextWriter output = new StreamWriter(oname);
				Pass2(aname, unmanaged, output);
				output.Close();
				asmargs.Add(oname);
			} catch {
				Warning("can't write {0}", oname);
			}
		}
		if (aname != String.Empty)
			asmargs.Add("-out=" + aname);
		ILasm.Run(asmargs, verbose, Console.Out);
		return 0;
	}
	static void ListUndefined() {
		foreach (DictionaryEntry e in Symbols.refs)
			if (!Symbols.defs.Contains((string)e.Key))
				Console.Error.WriteLine(e.Key);
	}
	/*
	Pass1:
	for each input file do
		for each line in the file do
			if line is call signature or loads a function pointer then
				add signature.name to R
				append signature to entry in R for signature.name
			else if line is .method ... signature ...
				add signature.name, signature to D
				remove name from R
			else if line is //$$name instructions
				add instructions to body for initialization function $$name
			else if line is ldsfld int8 __is_name_unmanaged
				add name to unmanaged, if necessary
	*/
	static void Pass1(string fname, TextReader r, ArrayList unmanaged) {
		string line;
		while ((line = r.ReadLine()) != null) {
			string name, signature;
			if (line.StartsWith("call ") || line.StartsWith("ldftn ")) {
				if (FindSignature(line, out name, out signature))
					Symbols.Ref(name, signature);
			} else if (line.StartsWith(".method")) {
				if (FindSignature(line, out name, out signature)) {
					if (Symbols.defs.Contains(name))
						Warning("\"{0}\" is multiply defined", name);
					Symbols.Def(name, signature);
				}
			} else if (line.StartsWith("//$$")) {
				int i = line.IndexOf(' ');
				Symbols.Func(line.Substring(2, i-2), line.Substring(i + 1, line.Length-i-1));
			} else if (line.StartsWith("ldsfld int8 __is_unmanaged_")) {
				name = line.Substring("ldsfld int8 __is_unmanaged_".Length);
				if (!unmanaged.Contains(name))
					unmanaged.Add(name);
			}
		}
	}
	static bool FindSignature(string line, out string name, out string signature) {
		name = signature = null;
		if (line.IndexOf("instance") >= 0 || line.IndexOf("rtspecialname") >= 0)
			return false;
		string[] args = line.Split(null);
		for (int i = 1; i < args.Length; i++) {
			int n = args[i].IndexOf('(');
			if (n > 0) {
				name = args[i].Substring(0, n);
				if (name[0] == '\'')
					name = name.Substring(1, name.Length-2);
				int k, j = i - 1;
				if (j - 1 >= 0 && args[j-1] == "valuetype")
					j--;
				if (j - 1 >= 0 && args[j-1] == "vararg")
					j--;
				for (k = i; k < args.Length; k++)
					if (args[k].IndexOf(')') >= 0)
						break;
				signature = String.Join(" ", args, j, k - j + 1);
				if ((k = signature.IndexOf(",...")) >= 0)
				    signature = signature.Substring(0, k) + ")";
				return true;
			}
		}
		return false;
	}
	/*
	Pass2:
	emit assembly header
	for each initialization function f do
		emit method header for f
		emit method body for f
	emit method header for $Main
	emit library initializations
	emit calls to initialization functions
	emit call to appropriate main()
	emit call to exit()
	for each external function f do
		emit method declaration for f
	for each potentially unmanaged function f do
		emit __is_unmanaged field for f
		initialize the field to 0/1 if f is managed/unmanaged
	*/
	static void Pass2(string aname, ArrayList unmanaged, TextWriter w) {
		w.WriteLine(".assembly '{0}' {{}}", aname);
		foreach (DictionaryEntry e in Symbols.funcs) {
			w.WriteLine(".method public hidebysig static void {0}() cil managed {{", e.Key);
			foreach (string inst in (ArrayList)e.Value)
				w.WriteLine("{0}", inst);
			w.WriteLine("ret\n}");
		}
		w.WriteLine(
@".method public hidebysig static void $Main(string[] argv) cil managed {
.entrypoint
.maxstack 3
ldsflda void* _caller
call void '_setcallerp'(void*)"	);
		foreach (DictionaryEntry e in Symbols.funcs)
			w.WriteLine("call void {0}()", e.Key);
		ArrayList sigs = Symbols.defs["main"];
		if (sigs == null || sigs.Count == 0)
			Warning("missing definition for \"{0}\"", "main");
		else {
			if (sigs.Count > 1)
				Warning("\"{0}\" is multiply defined", "main");
			switch ((string)sigs[0]) {
			case "void 'main'(int32,void*)":
			case "int32 'main'(int32,void*)":
				w.WriteLine(
@"ldarg 0
ldlen
ldc.i4 1
add
dup
ldc.i4 1
sub
ldarg 0
call void* __echo(int32,string[])
call {0}", sigs[0]);
				if (((string)sigs[0]).StartsWith("int"))
				    w.WriteLine("pop");				
				Symbols.Extern("__echo", "liblcc.dll");
				Symbols.Def("__echo", "void* __echo(int32,string[])");
				break;
			case "int32 'main'()":
				w.WriteLine("call {0}", sigs[0]);
				w.WriteLine("pop");
				break;
			case "void 'main'()":
				w.WriteLine("call {0}", sigs[0]);
				break;
			default:
				Warning("\"{0}\" is an invalid signature for \"main\"", sigs[0]);
				break;
			}
		}
		w.WriteLine(
@"ldc.i4 0
call void exit(int32)
ret
}
.field public static void* _caller at $_caller
.data $_caller = int32 (0)"		);
		foreach (DictionaryEntry e in Symbols.externs)
			foreach (string sig in Symbols.defs[(string)e.Key])
				w.WriteLine(
".method public hidebysig static pinvokeimpl(\"{0}\" ansi cdecl) {1} native unmanaged preservesig {{}}",
							e.Value, sig);
		foreach (string f in unmanaged) {
			w.WriteLine(".field public static int8 __is_unmanaged_{0} at $_{0}", f);
			w.WriteLine(".data $_{0} = int8 ({1})", f, Symbols.externs.Contains(f) ? 1 : 0);
		}
	}
}
