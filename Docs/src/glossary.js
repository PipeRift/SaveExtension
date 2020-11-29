let Glossary = {
  create: (config) => {
    this.config = config;

    let loadGlossary = async () => {
      return fetch('_glossary.md')
      .then(function(data){
        return data.text();
      })
      .then(function(text){
        window.$docsify.terms = {};

        let termTexts = text.split('#####');
        termTexts.forEach((termText) => {
          if (termText.length <= 5) { // If it can't possibly contain a term, skip
            return;
          }

          // Parse the term and its content
          let title = termText.split('\n', 1)[0].trim();
          let id = title.toLowerCase().replace(' ','-');
          let content = termText.substr(termText.indexOf('\n') + 1).trim();
          content = content.replace("\r\n\r\n", "\n");
          if (content.length > 200) {
            content = content.substring(0, 200) + "...";
          }
          window.$docsify.terms[title] = {
            id: id,
            content: content
          };
        });
      });
    };

    let addLinks = (content,next,terms) => {
      for (let term in terms) {
        console.log(term);

        let regex = new RegExp(`\\b${term}\\b`,'ig');
        content = content.replace(regex, (match) => {
          let termData = terms[term];
          return `[${match}*](/_glossary?id=${termData.id} "${termData.content}")`;
        });
      }
      next(content);
    };

    return (hook, vm) => {
      hook.beforeEach(function(content,next)
      {
        if(window.location.hash.match(/_glossary/g)) {
          next(content);
          return;
        }

        if (!window.$docsify.terms) {
          // Glossary needs loading before creating the link
          loadGlossary().then(() => {
            addLinks(content, next, window.$docsify.terms);
          })
        } else {
          addLinks(content, next, window.$docsify.terms);
        }
      });
    };
  }
};