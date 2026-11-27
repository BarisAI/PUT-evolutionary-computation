#!/usr/bin/env python3
"""Plain A4 PDF report based on the saved results. Requires reportlab and cairosvg."""
import argparse
import csv
from pathlib import Path
import tempfile

import cairosvg
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.utils import ImageReader
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, Image


def data(path):
    with path.open(newline='') as f:
        return list(csv.DictReader(f))


def main():
    arg=argparse.ArgumentParser()
    arg.add_argument('--results', type=Path, default=Path('results'))
    arg.add_argument('--output', type=Path, default=Path('Assignment_8_Report.pdf'))
    args=arg.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    results=args.results
    metrics=data(results/'metrics.csv')
    optima=data(results/'optima_summary.csv')
    styles=getSampleStyleSheet()
    styles.add(ParagraphStyle(name='StudentTitle',fontName='Helvetica-Bold',fontSize=15,leading=20,spaceAfter=12,alignment=TA_LEFT))
    styles.add(ParagraphStyle(name='StudentSub',fontName='Helvetica-Bold',fontSize=11,leading=14,spaceBefore=13,spaceAfter=5))
    styles.add(ParagraphStyle(name='StudentBody',fontName='Helvetica',fontSize=9.4,leading=14,spaceAfter=7))
    styles.add(ParagraphStyle(name='StudentSmall',fontName='Helvetica',fontSize=8,leading=11,spaceAfter=4))
    P=lambda s: Paragraph(s,styles['StudentBody'])
    H=lambda s: Paragraph(s,styles['StudentSub'])
    story=[]
    story.append(Paragraph('Assignment 8 - Global convexity tests',styles['StudentTitle']))
    story.append(P('Baris Surmelioglu (152048) and Ozan Polat (150918)<br/>Evolutionary Computation - Poznan University of Technology'))
    story.append(H('1. How we generated the local optima'))
    story.append(P('For TSPA and TSPB we generated 1,000 random starting tours and applied randomized first-improvement Greedy Local Search to each. The neighborhood combines 2-opt edge exchanges and selected/unselected node replacement. The search stops only when no improving move remains.'))
    story.append(P('Every tour has 100 unique selected vertices out of 200. We used the original rounded-distance objective plus the selected-node costs. The random generator seeds were 20260801 for TSPA and 20260901 for TSPB.'))
    story.append(H('2. How similarity was measured'))
    story.append(P('We counted (a) common undirected edges, including the closing edge, and (b) common selected nodes. We used three references: the average overlap with all other 999 sampled local optima; overlap with the best of the 1,000 sampled optima; and overlap with the previous best ILS solution. The best-sample plot excludes the reference sample itself, leaving 999 points.'))
    story.append(P('For average overlap, each feature contributes frequency - 1, divided by 999. This avoids counting the tour itself. Every chart uses objective value on the x-axis and similarity on the y-axis. Pearson r is computed from exactly the plotted points.'))
    story.append(H('3. Local-optimum sample statistics'))
    table_rows=[['Instance','Runs','Min.','Average','Max.','External ILS']]
    for row in optima:
        table_rows.append([row['instance'],row['n'],f"{int(row['minimum']):,}",f"{float(row['mean']):,.3f}",f"{int(row['maximum']):,}",f"{int(row['previous_ils_objective']):,}"])
    table=Table(table_rows,colWidths=[62,47,69,105,65,112],hAlign='LEFT')
    table.setStyle(TableStyle([
      ('FONTNAME',(0,0),(-1,0),'Helvetica-Bold'),('FONTSIZE',(0,0),(-1,-1),8.5),
      ('BOTTOMPADDING',(0,0),(-1,-1),7),('TOPPADDING',(0,0),(-1,-1),6),
      ('LINEBELOW',(0,0),(-1,0),0.7,colors.black),('LINEBELOW',(0,-1),(-1,-1),.3,colors.grey),
    ]))
    story.append(table)
    story.append(Spacer(1,8))
    story.append(P('The external ILS references were produced independently of these 1,000 runs. Both are better than the best sampled Greedy local optimum. All 2,000 sampled tours have different selected-node sets.'))
    story.append(PageBreak())
    story.append(H('4. Pearson correlation coefficients'))
    story.append(P('Each coefficient describes the association between tour cost and overlap. Negative r means lower-cost local optima tend to be more similar. No regression or global-optimality assumption is made.'))
    table_rows=[['Instance','Similarity','Reference','N','Pearson r']]
    labels={'edges':'Common edges','nodes':'Selected nodes'}
    names={'average_to_others':'Average to others','best_of_1000':'Best of 1,000','best_previous_ILS':'Previous best ILS'}
    for row in metrics:
        table_rows.append([row['instance'],labels[row['measure']],names[row['reference']],row['points'],f"{float(row['pearson_r']):+.3f}"])
    table=Table(table_rows,colWidths=[67,107,141,39,73],hAlign='LEFT')
    table.setStyle(TableStyle([
      ('FONTNAME',(0,0),(-1,0),'Helvetica-Bold'),('FONTSIZE',(0,0),(-1,-1),8.3),
      ('BOTTOMPADDING',(0,0),(-1,-1),8),('TOPPADDING',(0,0),(-1,-1),8),
      ('LINEBELOW',(0,0),(-1,0),.7,colors.black),
      ('LINEBELOW',(0,6),(-1,6),.5,colors.grey),
    ]))
    story.append(table)
    story.append(Spacer(1,10))
    story.append(H('5. Main observations'))
    story.append(P('All 12 coefficients are negative. The trend appears for both types of similarity and both problem instances. For both instances, the overlap in selected nodes with the previous ILS solution has a stronger negative correlation than the overlap in edges with that reference. The strength depends on the chosen measure and reference.'))
    story.append(P('This is evidence of a fitness-similarity relationship in our samples, not proof of global convexity. A strong correlation does not show that the global optimum has been found.'))
    story.append(H('6. Verification'))
    story.append(P('We checked all 2,000 objective values independently against the input coordinates. A separate C++ checker confirmed that every tour is feasible and locally optimal under both 2-opt and node replacement. Unit tests checked undirected edge orientation, node overlap, and excluding a solution from its own average. The average-overlap frequency method was also compared with direct pairwise intersections.'))
    story.append(PageBreak())
    with tempfile.TemporaryDirectory() as td:
        for idx,instance in enumerate(['TSPA','TSPB']):
            for mi,measure in enumerate(['edges','nodes']):
                for ri,ref in enumerate(['average_to_others','best_of_1000','best_previous_ILS']):
                    svg_path=results/'charts'/f'{instance}_{measure}_{ref}.svg'
                    png_path=Path(td)/f'{instance}_{measure}_{ref}.png'
                    cairosvg.svg2png(url=str(svg_path),write_to=str(png_path),output_width=1480)
                    title=f'{instance} - {labels[measure]} - {names[ref]}'
                    story.append(Paragraph(title,styles['StudentSmall']))
                    image=Image(str(png_path),width=490,height=312)
                    story.append(image)
                    story.append(Spacer(1,6))
                    chart_number=idx*6+mi*3+ri
                    if chart_number % 2 == 1 and chart_number != 11:
                        story.append(PageBreak())
        def footer(canvas,doc):
            canvas.setFont('Helvetica',8)
            canvas.drawRightString(A4[0]-42,29,f'{doc.page}')
        pdf=SimpleDocTemplate(str(args.output),pagesize=A4,rightMargin=47,leftMargin=47,topMargin=40,bottomMargin=40,
                              title='Assignment 8 - Global convexity tests',author='Baris Surmelioglu and Ozan Polat')
        pdf.build(story,onFirstPage=footer,onLaterPages=footer)
    print('Saved',args.output)


if __name__=='__main__':main()
