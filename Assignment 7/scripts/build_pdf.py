#!/usr/bin/env python3
"""Build the PDF from the exact final GitHub REPORT.md, with vector tour graphics."""
import subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parents[1]
HTML=HERE/'report_preview.html'
OUTPUT=Path('/mnt/data/Assignment_7_Evolutionary_Computation_Report.pdf')
subprocess.run(['pandoc','REPORT.md','-f','gfm','-t','html5','--standalone',
                '--metadata','title=Assignment 7 - Large Neighborhood Search',
                '-c','scripts/report_style.css','-o',str(HTML)],cwd=HERE,check=True)
subprocess.run(['weasyprint',str(HTML),str(OUTPUT)],cwd=HERE,check=True)
print(f'PDF created: {OUTPUT} ({OUTPUT.stat().st_size:,} bytes)')
