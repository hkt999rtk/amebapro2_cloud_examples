#!/usr/bin/env python3
"""Copy the canonical guide into the Developer Docs translation/build pipeline."""
import argparse
from pathlib import Path
import shutil
p=argparse.ArgumentParser();p.add_argument('--admin',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[1]
shutil.copyfile(root/'docs/developer/protwo-cloud-examples.en.md',a.admin/'web/content/developer-docs/protwo-cloud-examples.en.md')
