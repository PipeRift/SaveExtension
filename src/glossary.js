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
          if (termText.length <= 1) { // If it can't possibly contain a term, skip
            return;
          }

          // Parse the term and its content
          let titles = termText.split('\n', 1)[0].split(',');
          if(titles.length <= 0) {
            return;
          }
          let id = titles[0].trim().toLowerCase().replace(' ','-');
          let content = termText.substr(termText.indexOf('\n') + 1).trim();
          content = content.replace("\r\n\r\n", "\n");
          if (content.length > 200) {
            content = content.substring(0, 200) + "...";
          }

          titles.forEach((title)=> {
            window.$docsify.terms[title.trim()] = {
              id: id,
              content: content
            };
          });
        });
      });
    };

    let addLinks = (content,next,terms) => {
      let lines = content.split('\n');

      for (let term in terms) {
        let regex = new RegExp(`\\b${term}\\b`,'ig');

        let isBlock = false;
        lines.forEach((line, index) => {
          // Ignore titles
          if(line.startsWith('#')) {
            return;
          }

          // Ignore blocks of code or graphs
          if(line.startsWith('```')) {
            isBlock = !isBlock;
            return;
          } else if(isBlock) {
            return;
          }

          lines[index] = line.replace(regex, (match) => {
            let termData = terms[term];
            return `[${match}*](/_glossary?id=${termData.id} "${termData.content}")`;
          });
        });
      }
      content = lines.join('\n');
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