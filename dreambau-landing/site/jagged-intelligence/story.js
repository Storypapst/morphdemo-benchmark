// The stage and audio share the runtime's clock, including when the visitor skips.
(() => {
  const dream = window.Dream;
  const production = dream.state.id;
  const cut = [...document.querySelectorAll('[data-cut]')].find(link => link.dataset.cut === production);
  if (cut) {
    cut.setAttribute('aria-current', 'true');
    document.getElementById('replay').href = cut.href;
  }
  const phase = document.getElementById('phase-label');
  const progress = document.getElementById('progress');
  const time = document.getElementById('time');
  const labels = ['01 / DIE KANTE', '02 / DER BRÜCKENBAU', '03 / DER ÜBERGANG', '03 / DIE BRÜCKE STEHT'];
  let last = '';
  dream.onTime = () => {
    const { mode, T, error } = dream.state;
    const def = dream.prods[production];
    if (!def || error) return;
    const seconds = mode === 'still' ? def.dur : Math.min(def.dur, Math.floor(T));
    const done = mode === 'still' || document.documentElement.classList.contains('done');
    const label = labels[done ? 3 : T >= def.cross ? 2 : T >= def.build ? 1 : 0];
    const clock = String(seconds).padStart(2, '0') + ' / ' + def.dur + ' S';
    const key = label + clock;
    if (key !== last) {
      last = key;
      phase.textContent = label;
      time.textContent = clock;
      progress.max = def.dur;
      progress.value = seconds;
    }
  };
})();
