
'use strict';

function cliento(ready) {
    window.addEventListener('DOMContentLoaded', () => {
        ready($);
    });

    return $;

    function $(lmnt) {
        if (lmnt = document.querySelector(lmnt)) {
            lmnt._ = _;
            return lmnt;
        }
    }

    function _(selector, ...properties) {
        return (([, tagname, traits]) => {
            const child = document.createElement(tagname);
            const parse = {
                number: adopt,
                string: adopt,
                object(attributes) {
                    for (const [attribute, value] of Object.entries(attributes)) {
                        if (typeof value === 'object') {
                            for (const [property, is] of Object.entries(value)) {
                                child[attribute][property] = is;
                            }
                        } else {
                            child.setAttribute(attribute, value);
                        }
                    }
                }
            };

            if (traits) {
                traits.split('.').forEach(trait => {
                    const [name, id] = trait.split('#');
                    if (id) child.setAttribute('id', id);
                    if (name) child.classList.add(name);
                });
            }

            properties.forEach(arg => {
                (type => {
                    parse[type] && parse[type](arg);
                })(typeof arg);
            });

            child._ = _;

            return this.appendChild(child);

            function adopt(value) {
                child.innerText = value;
            }
        })(selector.match(/^([a-z1-5]+)(.+)?$/i));
    }
}