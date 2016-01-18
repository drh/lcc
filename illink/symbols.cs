// Copyright Microsoft Corporation. All rights reserved.
// $Id: symbols.cs#3 2002/08/30 10:25:34 REDMOND\\drh $

using System;
using System.IO;
using System.Collections;

struct Pair {
	public string name, file;
	public Pair(string name, string file) {
		this.name = name; this.file = file;
	}
}

class Dictionary : Hashtable {
	public Dictionary() {}
	public void Add(string name, ArrayList sigs) { base.Add(name, sigs); }
	public ArrayList this[string index] {
		get { return (ArrayList)base[index]; }
		set { base[index] = (ArrayList)value; }
	}
}

class Symbols {
	public static Dictionary defs = new Dictionary();
	public static Dictionary refs = new Dictionary();
	public static Dictionary funcs = new Dictionary();
	public static Hashtable externs = new Hashtable();
	public static void Def(string name) {
		if (!defs.Contains(name)) {
			defs.Add(name, new ArrayList());
			Log.WriteLine("Def: adding \"{0}\" to defs", name);			
		}
		if (refs.Contains(name)) {
			ArrayList sigs = refs[name];
			refs.Remove(name);
			Log.WriteLine("Def: removing \"{0}\" from refs", name);						
			foreach (string sig in sigs)
				Def(name, sig);
		}
	}
	public static void Def(string name, string signature) {
		Def(name);
		ArrayList sigs = defs[name];
		if (!sigs.Contains(signature)) {
			sigs.Add(signature);
			Log.WriteLine("Def: adding \"{0}\" for defs[\"{1}\"]", signature, name);						
		}
	}
	public static void Ref(string name, string signature) {
		if (defs.Contains(name))
			Def(name, signature);
		else {
			ArrayList sigs = refs[name];
			if (sigs == null) {
				sigs = new ArrayList();
				refs.Add(name, sigs);
				Log.WriteLine("Ref: adding \"{0}\" to refs", name);
			}
			if (!sigs.Contains(signature)) {
				sigs.Add(signature);
				Log.WriteLine("Ref: adding \"{0}\" for refs[\"{1}\"]", signature, name);						
			}
		}				
	}
	public static void Extern(string name, string file) {
		if (!externs.Contains(name)) {
			externs.Add(name, file);
			Log.WriteLine("Extern: adding \"{0}[{1}]\" to externs", name, file);
		}
	}
	public static void Func(string name, string instruction) {
		ArrayList body = funcs[name];
		if (body == null) {
			body = new ArrayList();
			funcs.Add(name, body);
			Log.WriteLine("Func: adding \"{0}\" to funcs", name);
		}
		body.Add(instruction);
		Log.WriteLine("Func: adding \"{0}\" to funcs[\"{1}\"]", instruction, name);
	}
}
