import { tmpdir } from "node:os";
import { sep } from "node:path";
import { mkdtempSync, existsSync, mkdtemp } from "fs";

const util = require("util");
const exec = util.promisify(require("child_process").exec);

const tmpDir = tmpdir();

function parseInput() {
  const argv = require("minimist")(process.argv.slice(2));
  delete argv["_"];

  const s3Paths = ["input-instance-folder", "output-folder"];

  for (const s3Path of s3Paths) {
    const path = argv[s3Path];
    if (!path) {
      throw new Error(`Missing required attribute ${s3Path}`);
    }
    if (!path.startsWith("s3://")) {
      throw new Error(
        `${s3Path} should point to a location in S3. Example: s3://bucket_name/key/`,
      );
    }
    if (!path.endsWith("/")) {
      throw new Error(`${s3Path} should end with a trailing slash`);
    }
  }
  return argv;
}

async function run(params: any) {
  const sourceFolder = mkdtempSync(`${tmpDir}${sep}`);
  const targetFolder = sourceFolder + "/results";

  const newArgs = { ...params };
  newArgs["input-instance-folder"] = sourceFolder;
  newArgs["output-folder"] = targetFolder;
  newArgs["sim"] = "K";
  newArgs["nsims"] = 2;
  newArgs["seed"] = 123;
  newArgs["nthreads"] = 7;
  newArgs["fmc"] = 66;
  newArgs["scenario"] = 2;
  newArgs["weather"] = "rows";

  const source = params["input-instance-folder"];
  const target = params["output-folder"];

  const args: string[] = [];

  for (const key of Object.keys(newArgs)) {
    args.push(`--${key} ${newArgs[key]}`);
  }

  console.log(`starting download ${source}`);
  await exec(`aws s3 cp ${source} ${sourceFolder} --recursive`);
  console.log(`downloaded ${source}`);

  /** 
   * ./Cell2Fire.Darwin.x86_64 
   * --final-grid 
   * --grids 
   * --sim K 
   * --nsims 2 
   * --seed 123 
   * --nthreads 7 
   * --fmc 66 
   * --scenario 2 
   * --weather rows 
   * --input-instance-folder /private/var/folders/0h/lvjhgtts0x32217c7ngfxfn00000gn/T/processing_CngDaM/04acfd74040d4cf2b0b572eb457cf80b/InstanceDirectory 
   * --output-folder /private/var/folders/0h/lvjhgtts0x32217c7ngfxfn00000gn/T/processing_CngDaM/04acfd74040d4cf2b0b572eb457cf80b/InstanceDirectory/results 
  */

  const cmd = `Cell2Fire --final-grid --grids ${args.join(" ")}`;
  console.log({ cmd });

  const output = await exec(cmd);
  console.log({ output });

  const sampleScarFile = `${targetFolder}/Grids/Grids1/ForestGrid0.csv`;
  if (!existsSync(sampleScarFile)) {
    throw new Error(`Invalid sample scar file ${sampleScarFile}`)
  }

  /**
   * qgis_process run fire2a:scar 
   * --distance_units=meters 
   * --area_units=m2 
   * --ellipsoid=EPSG:7030 
   * --BaseLayer=/Users/mabahamo/Downloads/k/elevation.asc 
   * --SampleScarFile=/Users/mabahamo/Downloads/k/results/Grids/Grids1/ForestGrid0.csv 
   * --ScarRaster=/tmp/finalScar.tif 
   * --ScarPolygon=/tmp/scars.gpkg 
   * --BurnProbability=TEMPORARY_OUTPUT
   * 
   * { 'BaseLayer' : '/Users/mabahamo/Downloads/k/elevation.asc', 
   * 'BurnProbability' : 'TEMPORARY_OUTPUT', 
   * 'SampleScarFile' : '/Users/mabahamo/Downloads/k/results/Grids/Grids1/ForestGrid0.csv', 
   * 'ScarPolygon' : '/tmp/scars.gpkg', 
   * 'ScarRaster' : '/tmp/finalScar.tif' }
   */

  const scarPolygon = `${targetFolder}/scars.gpkg`;
  const fireScarCmd = `qgis_process run fire2a:scar --BaseLayer=${sourceFolder}/fuels.asc --BurnProbability=TEMPORARY_OUTPUT --SampleScarFile=${sampleScarFile} --ScarPolygon=${scarPolygon} --ScarRaster=${targetFolder}/scarRaster.tif`
  console.log({fireScarCmd});

  const fireScar = await exec(fireScarCmd);
  console.log(`out: ${fireScar.stdout}\n\nerr: ${fireScar.stderr}`);

  const fireScarPngCmd = `python3 /usr/local/Cell2FireWrapper/gpkg_to_png.py ${scarPolygon} ${targetFolder}/scars.png`
  const fireScarPng = await exec(fireScarPngCmd);
  console.log({fireScarPng});

  //TODO: These files should be compressed before uploading to S3

  console.log(`starting upload ${sourceFolder} -> ${target}`);
  const ls = await exec(`find ${sourceFolder}`);
  console.log("ls: " + ls.stdout);
  await exec(`aws s3 cp ${sourceFolder} ${target} --recursive`);
  console.log(`uploaded to ${target}`);
}

const input = parseInput();
console.log("argv: ", input);

run(input).then(() => {
  console.log("done");
});
