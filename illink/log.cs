// Copyright Microsoft Corporation. All rights reserved.
// $Id: log.cs#2 2002/08/30 10:25:32 REDMOND\\drh $

using System;
using System.IO;

class Log {
	public static TextWriter log = TextWriter.Null;
	static void close() {
		if (log != Console.Out && log != Console.Error)
			log.Close();
	}
	static void echo(string[] argv) {
		log.Write(argv[0]);
		for (int i = 1; i < argv.Length; i++)
			log.Write(" {0}", argv[i]);
		log.WriteLine();
	}
	public static void New(string file, string[] argv) {
		New(file);
		echo(argv);
	}
	public static void New(string file) {
		close();
		log.WriteLine(DateTime.Now);
	}
	public static void New(TextWriter w, string[] argv) {
		New(w);
		echo(argv);
	}
	public static void New(TextWriter w) {
		close();
		log = w;
		log.WriteLine(DateTime.Now);
	}
	public static void Write(string fmt, params string[] args) {
		log.WriteLine(fmt, args);
		log.Flush();
	}
	public static void WriteLine(string fmt, params string[] args) {
		log.WriteLine(fmt, args);
		log.Flush();
	}
}
