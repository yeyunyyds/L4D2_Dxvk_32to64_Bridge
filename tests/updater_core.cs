// SPDX-License-Identifier: MIT
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Text;
using LdbUpdater;
class UpdaterTests {
  const string Build="l4d2-1.3.2+1111111111111111";
  static int assertions;
  static void Check(bool value,string reason){++assertions;if(!value)throw new Exception(reason);}
  static void Reject(Action action,string reason){bool failed=false;try{action();}catch{failed=true;}Check(failed,reason);}
  static string Temp(){string p=Path.Combine(Path.GetTempPath(),"ldb-updater-test-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(Core.HostDir(p));return p;}
  static byte[] Pe(int machine,string tag){byte[] result=new byte[512];result[0]=(byte)'M';result[1]=(byte)'Z';BitConverter.GetBytes(64).CopyTo(result,60);result[64]=(byte)'P';result[65]=(byte)'E';BitConverter.GetBytes((ushort)machine).CopyTo(result,68);result[86]=0x20;Encoding.ASCII.GetBytes(Build+tag).CopyTo(result,128);return result;}
  static void Initial(string root){foreach(var slot in Core.Slots)File.WriteAllBytes(Core.Target(root,slot),slot=="readme"?Encoding.UTF8.GetBytes("old readme"):Pe(slot=="host64"?0x8664:0x14c,"old"));File.WriteAllText(Path.Combine(Core.HostDir(root),"bridge.conf"),"PRESERVE-CONFIG");File.WriteAllText(Path.Combine(Core.HostDir(root),"d3d9vk_x64.dll"),"PRESERVE-DXVK");File.WriteAllText(Path.Combine(Core.HostDir(root),"retention.db"),"PRESERVE-DB");}
  static Dictionary<string,string> Hashes(string root){return Core.Slots.ToDictionary(s=>s,s=>Core.HashFile(Core.Target(root,s)));}
  static void Preserved(string root){Check(File.ReadAllText(Path.Combine(Core.HostDir(root),"bridge.conf"))=="PRESERVE-CONFIG","conf changed");Check(File.ReadAllText(Path.Combine(Core.HostDir(root),"d3d9vk_x64.dll"))=="PRESERVE-DXVK","backend changed");Check(File.ReadAllText(Path.Combine(Core.HostDir(root),"retention.db"))=="PRESERVE-DB","DB changed");}
  static string Package(string root,string version,string extra,bool corrupt){
    string path=Path.Combine(root,"test-"+Guid.NewGuid().ToString("N")+".vpk");var files=new List<Payload>();var bodies=new Dictionary<string,byte[]>();
    foreach(var slot in Core.Slots){string source=slot=="client"?"bin/dxvk_d3d9.dll":slot=="host32"?"bin/.l4d2bridge/L4D2Bridge32.exe":slot=="host64"?"bin/.l4d2bridge/L4D2Bridge64.exe":"UPDATE-README.md";byte[] b=slot=="readme"?Encoding.UTF8.GetBytes("new readme 中文"):Pe(slot=="host64"?0x8664:0x14c,"new");bodies[source]=b;using(var m=new MemoryStream(b))files.Add(new Payload{slot=slot,source=source,bytes=b.Length,sha256=Core.Hash(m)});}
    if(corrupt)bodies[files[0].source][511]=4;
    using(var zip=ZipFile.Open(path,ZipArchiveMode.Create)){
      foreach(var body in bodies)using(var s=zip.CreateEntry(body.Key).Open())s.Write(body.Value,0,body.Value.Length);
      using(var w=new StreamWriter(zip.CreateEntry("VERSION").Open(),new UTF8Encoding(false)))w.Write(version);
      using(var w=new StreamWriter(zip.CreateEntry("UPDATE-MANIFEST.json").Open(),new UTF8Encoding(false)))w.Write(Core.Json.Serialize(new Manifest{schema=1,product="LDB",mode="local-test-editable-version",build_id=Build,files=files.ToArray()}));
      if(extra!=null)using(var w=new StreamWriter(zip.CreateEntry(extra).Open()))w.Write("bad");
    }return path;
  }
  static Candidate Extract(string root,string package){return Core.Extract(root,package,"1.3.2",Core.HashFile(package),true);}
  static void Clean(string root){Directory.Delete(root,true);}
  static int Main(string[] args){
    if(args.Length==4&&args[0]=="crash"){
      var c=Extract(args[1],args[2]);int step=Int32.Parse(args[3]);Core.Install(args[1],c,i=>{if(i==step)Process.GetCurrentProcess().Kill();});return 2;
    }
    Check(Core.VersionOf("1.3.10").CompareTo(Core.VersionOf("1.3.9"))>0,"Numeric version order");
    foreach(var bad in new[]{"1.3","1.3.2+hash","01.3.2","1.3.2-preview","999999999999.1.1"})Reject(()=>Core.VersionOf(bad),"Bad version accepted");
    string root=Temp();Initial(root);string package=Package(root,"1.3.3",null,false);
    Reject(()=>Core.Extract(root,package,"1.3.2",null,false),"Unsigned package accepted outside test mode");
    Check(Core.Extract(root,package,"1.3.3",null,true)==null,"Equal version prompted");
    Check(Core.Extract(root,package,"1.3.4",null,true)==null,"Older version prompted");
    Reject(()=>Core.Extract(root,package,"1.3.2",new string('0',64),true),"Source changed undetected");
    Reject(()=>Extract(root,Package(root,"1.3.3","../escape",false)),"Traversal accepted");
    Reject(()=>Extract(root,Package(root,"1.3.3","VERSION",false)),"Duplicate entry accepted");
    Reject(()=>Extract(root,Package(root,"1.3.3",null,true)),"Damaged binary accepted");
    var declined=Extract(root,package);var before=Hashes(root);Core.Cleanup(root,declined.Transaction);
    Check(Core.Slots.All(s=>before[s]==Core.HashFile(Core.Target(root,s))),"Decline changed installed files");
    Check(!File.Exists(Path.Combine(Core.HostDir(root),"LDBupdate-package.sha256")),"Decline persisted package hash");
    var accepted=Extract(root,package);Check(accepted!=null,"Decline suppressed next startup");Core.Install(root,accepted,null);
    Check(Core.Slots.All(s=>before[s]!=Core.HashFile(Core.Target(root,s))),"Successful install did not replace every payload");
    Check(File.ReadAllText(Path.Combine(Core.HostDir(root),"LDBupdate-package.sha256"))==Core.HashFile(package),"Successful package hash missing");Preserved(root);Check(!File.Exists(Core.JournalPath(root)),"Successful journal left pending");Clean(root);
    for(int step=1;step<=4;step++){
      root=Temp();Initial(root);before=Hashes(root);package=Package(root,"1.3.3",null,false);var c=Extract(root,package);int failAt=step;
      Reject(()=>Core.Install(root,c,i=>{if(i==failAt)throw new IOException("injected replacement failure");}),"Injected replace failure ignored");
      Check(Core.Slots.All(s=>before[s]==Core.HashFile(Core.Target(root,s))),"Rollback failed at step "+step);Preserved(root);Check(!File.Exists(Core.JournalPath(root)),"Rollback journal not cleared");Clean(root);
    }
    for(int step=1;step<=4;step++){
      root=Temp();Initial(root);before=Hashes(root);package=Package(root,"1.3.3",null,false);
      var start=new ProcessStartInfo(System.Reflection.Assembly.GetExecutingAssembly().Location,"crash \""+root+"\" \""+package+"\" "+step){UseShellExecute=false,CreateNoWindow=true};
      using(var child=Process.Start(start)){Check(child.WaitForExit(15000),"Crash fixture hung");Check(child.ExitCode!=0,"Crash fixture survived");}
      Check(File.Exists(Core.JournalPath(root)),"Crash lost recovery journal");Core.Recover(root);
      Check(Core.Slots.All(s=>before[s]==Core.HashFile(Core.Target(root,s))),"Crash recovery did not restore all files");Preserved(root);Clean(root);
    }
    root=Temp();Initial(root);File.Delete(Core.Target(root,"readme"));package=Package(root,"1.3.3",null,false);var missingReadme=Extract(root,package);
    Reject(()=>Core.Install(root,missingReadme,i=>{if(i==4)throw new IOException("injected");}),"Missing README rollback case");Check(!File.Exists(Core.Target(root,"readme")),"Rollback created old nonexistent README");Clean(root);
    root=Temp();Initial(root);package=Package(root,"1.3.3",null,false);var busy=Extract(root,package);before=Hashes(root);
    using(var locked=new FileStream(Core.Target(root,"host32"),FileMode.Open,FileAccess.Read,FileShare.None))Reject(()=>Core.Install(root,busy,null),"Locked target did not block install");
    Check(Core.Slots.All(s=>before[s]==Core.HashFile(Core.Target(root,s))),"Locked target left mixed install");Clean(root);
    root=Temp();Initial(root);package=Package(root,"\uFEFF1.3.3",null,false);var bom=Extract(root,package);Check(bom.Version=="1.3.3","Notepad UTF-8 BOM rejected");Core.Cleanup(root,bom.Transaction);Clean(root);
    foreach(bool damage in new[]{false,true}) {
      root=Temp();Initial(root);before=Hashes(root);package=Package(root,"1.3.3",null,false);var c=Extract(root,package);FileStream held=null;
      try { Core.Install(root,c,i=>{if(i==4)held=new FileStream(Path.Combine(Core.Stage(root,c.Transaction),"backup","client"),FileMode.Open,FileAccess.Read,FileShare.None);}); }
      finally { if(held!=null)held.Dispose(); }
      Check(File.Exists(Core.JournalPath(root)),"Failed post-commit cleanup discarded recovery marker");
      if(damage) {
        File.WriteAllText(Core.Target(root,"client"),"damaged after commit");
        int remaining=Directory.GetFiles(Core.Stage(root,c.Transaction),"*",SearchOption.AllDirectories).Length;
        Reject(()=>Core.Recover(root),"Committed corruption was silently accepted");
        Check(File.Exists(Core.JournalPath(root))&&Directory.GetFiles(Core.Stage(root,c.Transaction),"*",SearchOption.AllDirectories).Length==remaining,"Corruption discarded remaining recovery material");
        File.WriteAllBytes(Core.Target(root,"client"),Pe(0x14c,"new"));
      }
      Core.Recover(root);
      Check(Core.Slots.All(slot=>before[slot]!=Core.HashFile(Core.Target(root,slot))),"Committed recovery reverted a verified install");
      Check(!File.Exists(Core.JournalPath(root)),"Committed recovery did not finish");Preserved(root);Clean(root);
    }
    Console.WriteLine("UPDATER_CORE_PASS assertions="+assertions+" replacement-failure=4 crash-recovery=4");return 0;
  }
}
