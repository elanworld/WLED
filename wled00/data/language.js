
function loadLanguage()
{
  locip = localStorage.getItem('locIp')
  useLocalTranslation = localStorage.getItem('useLocalTranslation');
  languageJsUrl = localStorage.getItem('languageJsUrl');
  if (useLocalTranslation === 'true') {
    if (languageJsUrl) {
      const script = document.createElement('script');
      script.src = languageJsUrl;
      document.head.appendChild(script);
    } else {
      var languageLocal = (locip ? `http://${locip}` : '') + '/language_local.js';
      const script = document.createElement('script');
      script.src = languageLocal;
      document.head.appendChild(script);
    }
  }
  var url = (locip ? `http://${locip}` : '') + '/json/state';
  fetch(url, {
    method: 'get'
  })
    .then(res => {
      if (res.ok) {
        res.json()
          .then(function (reuslt) {
            if (reuslt.hasOwnProperty('useLocalTranslation')) {
              localStorage.setItem('useLocalTranslation', reuslt.useLocalTranslation)
            }
            if (reuslt.hasOwnProperty('languageJsUrl')) {
              localStorage.setItem('languageJsUrl', reuslt.languageJsUrl)
            }
          })
      }
    })
    .catch(function (error) {
      console.log(error)
    })
}

loadLanguage()



