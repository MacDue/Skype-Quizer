import React from 'react'

const nextPage = () => {
  fetch("/next").catch((error) => {
    alert("Failed to goto next page: " + error)
  })
}

class Controller extends React.Component {
  state = {
    categories_to_play: 5, questions_per_category: 10
  }

  updateCtp = (event) => {
    this.setState({categories_to_play: event.target.value});
  }

  updateQpc = (event) => {
    this.setState({questions_per_category: event.target.value});
  }

  resetQuiz = (event) => {
     const { categories_to_play, questions_per_category } = this.state
     fetch(`/reset/${categories_to_play}/${questions_per_category}`)
    .catch((error) => {
      alert("Failed to reset quiz: " + error)
    })
    event.preventDefault();
    alert("Reset!")
  }

  render = () => (
    <div>
      <h1>Ben's Skype Remote!</h1>
      <div>
        <button onClick={nextPage}>Next page</button>
      </div>
      <h2>New Quiz!</h2>
      <form onSubmit={this.resetQuiz}>
        <label>
          Categories to play:
          <input type="number"
            value={this.state.categories_to_play} onChange={this.updateCtp} />
        </label>
        <br/>
        <label>
          Questions per category:
          <input type="number"
            value={this.state.questions_per_category} onChange={this.updateQpc} />
        </label>
        <br/>
        <input type="submit" value="Reset"/>
      </form>
    </div>)
}

export default Controller;
