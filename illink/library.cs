// Copyright Microsoft Corporation. All rights reserved.
// $Id: library.cs#3 2002/12/10 15:09:01 REDMOND\\drh $

using System;
using System.IO;
using System.Collections;
using System.Diagnostics;
using System.Runtime.InteropServices;

class Library {
	class Lib {
		[DllImport("kernel32.dll")]
		static private extern int LoadLibrary(string file);
		[DllImport("kernel32.dll")]
		static private extern int GetProcAddress(int handle, string name);
		[DllImport("kernel32.dll")]
		static private extern void FreeLibrary(int handle);
		int handle;
		string file;
		public Lib(string file) {
			this.file = file;
			handle = 0;
		}
		public bool Open() {
			handle = LoadLibrary(file);
			return handle != 0;
		}
		public void Close() {
			//if (handle != 0)
			//	FreeLibrary(handle);
		}
		~Lib() {
			Close();
		}
		public bool Exports(string name) {
			if (handle == 0)
				throw new InvalidOperationException("Attempt to query a closed Dll");
			return GetProcAddress(handle, name) != 0;
		}
	}
	/*
	for each name in R do
		if file exports name then
			remember name, file
	for each remembered name, file do
		move name from R to D
		add (name, file) to externs
	*/
	public static void Search(string file) {
		Search(file, false, Console.Out);
	}
	public static void Search(string file, bool verbose, TextWriter w) {
		Lib lib = new Lib(file);
		if (lib.Open()) {
			if (verbose)
				w.WriteLine("searching {0}...", file);
			ArrayList todo = new ArrayList();
			foreach (DictionaryEntry e in Symbols.refs)
				if (lib.Exports((string)e.Key))
					todo.Add((string)e.Key);
			foreach (string name in todo) {
				if (verbose)
					w.WriteLine("   found {0}", name);
				Symbols.Extern(name, file);
				Symbols.Def(name);
			}
			lib.Close();
		} else if (verbose)
			illink.Warning("can't search '{0}'", file);
	}
}
