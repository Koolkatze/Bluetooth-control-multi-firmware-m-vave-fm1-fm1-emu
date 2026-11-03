import {test} from 'node:test';
import assert from 'node:assert/strict';
import {KNOB_SWEEP_LIMIT, FM1_KNOB_CENTERS, knobPercent, beginKnobGesture, moveKnobGesture} from './knob-sweep.js';
const deg = Math.PI / 180;
const place = (i, d, r=28) => {
  const [cx,cy]=FM1_KNOB_CENTERS[i];
  return [cx + Math.sin(d*deg)*r, cy - Math.cos(d*deg)*r];
};
test('exact 7:00..12:00..5:00 maps 0..50..100 on every FM-1 knob', () => {
  assert.equal(KNOB_SWEEP_LIMIT, 150*deg);
  for(let i=0;i<FM1_KNOB_CENTERS.length;i++) {
    const [x,y]=place(i,-150);
    const g=beginKnobGesture(i,x,y);
    assert.equal(g.percent,0);
    let clicks=0;
    for(let d=-149;d<=150;d++)clicks+=moveKnobGesture(g,...place(i,d));
    assert.equal(g.percent,100);
    assert.equal(clicks,100);
    assert.equal(knobPercent(0),50);
    for(let d=149;d>=-150;d--)clicks+=moveKnobGesture(g,...place(i,d));
    assert.equal(g.percent,0);
    assert.equal(clicks,0);
  }
});
test('bottom dead zone never wraps 100->0, stays clamped through 180 degrees',()=>{
  const g=beginKnobGesture(3,...place(3,140));
  let sum=0;
  for(const d of [150,160,170,179,181,190,200,209,210])
    sum+=moveKnobGesture(g,...place(3,d));
  assert.equal(g.percent,100);
  assert.equal(sum,knobPercent(150*deg)-knobPercent(140*deg));
  for(const d of [200,190,180,170,160,150,120,90,0,-150])
    moveKnobGesture(g,...place(3,d));
  assert.equal(g.percent,0);
});
test('knob centre ignored; each stationary pointer movement emits no spurious detents',()=>{
  const g=beginKnobGesture(6,...place(6,-30));
  assert.equal(moveKnobGesture(g,g.cx,g.cy),0);
  assert.equal(moveKnobGesture(g,...place(6,-30)),0);
  assert.equal(moveKnobGesture(g,...place(6,0)),10);
  assert.equal(moveKnobGesture(g,...place(6,0)),0);
});
test('direct forbidden-sector entry saturates without wrap or huge jumps',()=>{
  const g=beginKnobGesture(4,...place(4,150));
  assert.equal(moveKnobGesture(g,...place(4,-150)),0);
  assert.equal(g.percent,100);
  assert.equal(moveKnobGesture(g,...place(4,150)),0);
  assert.equal(g.percent,100);
});
