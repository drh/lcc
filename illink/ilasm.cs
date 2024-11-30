// Copyright Microsoft Corporation. All rights reserved.
// $Id: ilasm.cs#3 2002/08/30 10:25:17 REDMOND\\drh $

using System;
using System.IO;
using System.Collections;
using System.Text;
using System.Diagnostics;

class ILasm {
	public static void Run(ArrayList args, bool verbose, TextWriter w) {
		StringBuilder sb = new StringBuilder();
		if (args.Count > 0)
			sb.Append(args[0]);
		for (int i = 1; i < args.Count; i++)
			sb.AppendFormat(" {0}", args[i]);
		Process p = new Process();
		p.StartInfo.UseShellExecute = false;
		p.StartInfo.FileName = "ilasm";
		p.StartInfo.Arguments = sb.ToString();
		if (verbose)
			w.WriteLine("{0} {1}", p.StartInfo.FileName, p.StartInfo.Arguments);
		p.Start();
		p.WaitForExit();
	}
}
