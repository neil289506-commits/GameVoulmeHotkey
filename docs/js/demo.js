
let v=50,mu=false;
function ui(){const x=mu?0:v;vn.textContent=mu?'靜音':v+'%';vb.style.width=x+'%';vi.textContent=mu||v===0?'🔇':v<40?'🔈':v<70?'🔉':'🔊'}
const up=()=>{mu=false;v=Math.min(100,v+5);ui()},dn=()=>{v=Math.max(0,v-5);ui()},mt=()=>{mu=!mu;ui()};
vu.onclick=up;vd.onclick=dn;vm.onclick=mt;
gv.addEventListener('keydown',e=>{if(!e.altKey)return;if(e.key==='ArrowUp'){up()}else if(e.key==='ArrowDown'){dn()}else if(e.key==='Delete'){mt()}else return;e.preventDefault()});
ui();
