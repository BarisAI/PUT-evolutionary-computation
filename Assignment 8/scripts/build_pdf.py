#!/usr/bin/env python3
"""Render REPORT.md and its saved scatterplots as a plain A4 PDF.

Run from Assignment 8. Requires Pandoc on PATH and Python packages
weasyprint, beautifulsoup4, cairosvg and Pillow.
"""
import argparse
from io import BytesIO
from pathlib import Path
import re
import subprocess
import tempfile

from bs4 import BeautifulSoup
import cairosvg
from PIL import Image
from weasyprint import CSS, HTML

STYLE = '''
@page { size: A4; margin: 17mm 17mm 16mm;
  @bottom-center { content: counter(page); font: 9pt "DejaVu Sans"; color: #555; } }
body { font: 10.1pt/1.38 "DejaVu Sans", sans-serif; color: #161616; margin: 0; }
h1 { font-size: 19pt; line-height: 1.16; margin: 0 0 9pt; font-weight: 700; }
h2 { font-size: 14pt; line-height: 1.2; margin: 16pt 0 5pt; break-after: avoid; }
h3 { font-size: 11pt; line-height: 1.2; margin: 11pt 0 4pt; break-after: avoid; }
p { margin: 0 0 7pt; widows: 3; orphans: 3; }
pre { font: 8.15pt/1.29 "DejaVu Sans Mono", monospace; background: #f5f5f5;
  border: 0.5pt solid #d5d5d5; border-left: 2pt solid #727272;
  padding: 6pt 7pt; margin: 5pt 0 9pt; white-space: pre-wrap; break-inside: avoid; }
code { font-family: "DejaVu Sans Mono", monospace; font-size: 0.91em; }
pre code { font-size: 1em; }
table { border-collapse: collapse; width: 100%; font-size: 8.6pt; line-height: 1.22;
  margin: 5pt 0 10pt; break-inside: avoid; }
th, td { border-bottom: .45pt solid #c9c9c9; padding: 4.5pt 4pt; text-align: left;
  vertical-align: top; }
th { background: #e9e9e9; font-weight: 700; }
.chart-pair { break-before: page; break-inside: avoid; }
.chart-pair.first-pair { break-before: auto; }
.chart-block { break-inside: avoid; margin: 0 0 8pt; }
.chart-block h3 { margin: 0 0 3pt; font-size: 10pt; }
figure { margin: 0; text-align: center; }
figure img { display: block; width: 100%; height: auto; max-width: 100%; }
figcaption { display: none; }
.chart-pair.first-pair figure img { width: 91%; margin: 0 auto; }
'''


def chart_layout(html):
    soup = BeautifulSoup(html, 'html.parser')
    for heading in soup.find_all('h3'):
        if re.match(r'^TSP[AB] - ', heading.get_text()):
            figure = heading.find_next_sibling()
            if figure is None or figure.name != 'figure':
                raise ValueError(f'Missing chart: {heading.get_text()}')
            block = soup.new_tag('div', attrs={'class': 'chart-block'})
            heading.insert_before(block)
            block.append(heading.extract())
            block.append(figure.extract())
    blocks = soup.find_all('div', class_='chart-block')
    if len(blocks) != 12:
        raise ValueError(f'Expected 12 charts, found {len(blocks)}')
    for i in range(0, len(blocks), 2):
        classes = 'chart-pair' if i else 'chart-pair first-pair'
        pair = soup.new_tag('div', attrs={'class': classes})
        blocks[i].insert_before(pair)
        pair.append(blocks[i].extract())
        pair.append(blocks[i+1].extract())
    for heading in soup.find_all('h2'):
        if heading.get_text().startswith(('6. Scatterplots', '7. Discussion')):
            heading['style'] = 'break-before: page; margin-top: 0;'
    return str(soup)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, default=Path('REPORT.md'))
    parser.add_argument('--results', type=Path, default=Path('results'))
    parser.add_argument('--output', type=Path,
                        default=Path('Assignment_8_Evolutionary_Computation_Report.pdf'))
    args = parser.parse_args()
    markdown = args.report.read_text(encoding='utf-8')
    references = re.findall(r'\]\((results/charts/[^)]+)\.svg\)', markdown)
    if len(references) != 12 or len(set(references)) != 12:
        raise ValueError('REPORT.md must contain 12 distinct SVG chart references')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as directory:
        temp = Path(directory)
        for name in references:
            svg = args.results / 'charts' / (Path(name).name + '.svg')
            jpg = temp / (name + '.jpg')
            jpg.parent.mkdir(parents=True, exist_ok=True)
            png = cairosvg.svg2png(url=str(svg.resolve()), output_width=1480)
            Image.open(BytesIO(png)).convert('RGB').save(
                jpg, quality=88, optimize=True, subsampling=0)
        render_markdown = re.sub(r'\]\((results/charts/[^)]+)\.svg\)',
                                 r'](\1.jpg)', markdown)
        html = subprocess.run(['pandoc', '-f', 'markdown', '-t', 'html5', '--no-highlight'],
                              input=render_markdown, text=True, capture_output=True,
                              check=True).stdout
        content = chart_layout(html)
        document = ('<!doctype html><html><head><meta charset="utf-8">'
                    '<title>Assignment 8 - Global Convexity Tests</title>'
                    '<meta name="author" content="Baris Surmelioglu and Ozan Polat">'
                    '</head><body>' + content + '</body></html>')
        HTML(string=document, base_url=str(temp)).write_pdf(
            str(args.output), stylesheets=[CSS(string=STYLE)])
    print('Saved', args.output)


if __name__ == '__main__':
    main()
