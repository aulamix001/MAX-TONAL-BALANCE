"""Generate original provisional seeds, never measured reference material."""
import json, math
from pathlib import Path
master = ['All-Purpose','Pop','Dance Pop','Hip-Hop','Classic Rock','Alt Rock','Country','EDM','Drum & Bass','Dubstep','Ambient','Dark Cinematic','Acoustic/Singer-Songwriter','Funk/Soul','Sertanejo','Samba/Pagode','MPB','Forró','Gospel','Reggaeton','Jazz','Classical']
track = ['Lead Vocal','Backing Vocal','Kick','Snare','Toms','Overheads/Cymbals','Drum Bus','Bass (electric/synth)','Electric Guitar','Acoustic Guitar','Keys/Piano','Synth Pad','Strings/Brass','Percussion','FX/Ambience','Custom']
freqs = [20 * 1000 ** (i / 199) for i in range(200)]
# Low, body, presence, air boosts in dB. Original hand-authored synthetic profiles.
profiles = [(0,0,0,0),(0,-1,1,1),(2,-1,1,2),(3,1,-1,0),(1,2,1,-1),(1,1,2,0),(0,1,1,1),(3,-2,1,2),(4,-1,2,1),(5,-2,0,1),(-1,1,-2,2),(3,2,-2,-1),(-2,1,1,1),(2,1,0,1),(1,0,2,1),(1,2,1,2),(0,1,1,0),(2,1,2,1),(0,1,2,2),(4,0,0,1),(-1,2,0,-1),(-2,2,-1,0),(-4,2,3,2),(-4,1,0,2),(8,-2,2,-5),(-4,4,4,2),(4,3,2,-2),(-8,-3,1,6),(3,0,2,1),(8,-1,0,-6),(-3,3,4,-4),(-3,2,1,4),(-2,4,2,1),(-4,0,2,4),(-3,3,3,1),(-6,-1,4,3),(-8,-2,1,6),(0,0,0,0)]
def bump(f,c): return math.exp(-.5*(math.log2(f/c)/1.0)**2)
models=[]
for index,name in enumerate(master+track):
    group='master' if index < len(master) else 'track'
    shape=profiles[index]
    values=[-3*math.log2(f/1000)+sum(a*bump(f,c) for a,c in zip(shape,(65,240,3500,12000))) for f in freqs]
    offset=sum(v for f,v in zip(freqs,values) if 100<=f<=5000)/sum(100<=f<=5000 for f in freqs)
    values=[round(v-offset,4) for v in values]
    mono=name in ('Kick','Bass (electric/synth)','Lead Vocal')
    models.append(dict(schemaVersion=1,id=f'{group}-{index:02d}',name=name,category=group,provisional=True,provenance='Original parametric seed; not measured audio',frequencies_Hz=freqs,mean_dB=values,lo_dB=[v-2 for v in values],hi_dB=[v+2 for v in values],suggested=dict(vocalBalance=[-6,0],psr=[8,16] if group=='master' else [10,20],stereoWidth=[0,.15] if mono else [.15,.65]),vocalBalanceEnabled=group=='master',lowEndCrest=[8,16]))
root=Path(__file__).resolve().parents[1]
(root/'resources/targets/factory.json').write_text(json.dumps(models,ensure_ascii=False,indent=2)+'\n')
print(f'{len(master)} master + {len(track)} track = {len(models)} provisional presets')
